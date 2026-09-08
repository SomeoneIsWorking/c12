from __future__ import annotations

import hashlib
import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from tools.title_identity import IdentityError, authenticate_extraction

ROOT = Path(__file__).resolve().parents[1]


class TitleIdentityTests(unittest.TestCase):
    def setUp(self) -> None:
        scratch = ROOT / "scratch" / "tests"
        scratch.mkdir(parents=True, exist_ok=True)
        self.directory = tempfile.TemporaryDirectory(dir=scratch)
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.cnf = self.root / "SYSTEM.CNF"
        self.exe = self.root / "SCUS_946.66"
        self.cnf.write_text("BOOT = cdrom:\\SCUS_946.66;1\r\n", encoding="ascii")
        self.exe.write_bytes(b"synthetic title fixture")
        self.enterContext(
            patch("tools.title_identity.EXECUTABLE_SIZE", self.exe.stat().st_size)
        )
        self.enterContext(
            patch(
                "tools.title_identity.EXECUTABLE_SHA256",
                hashlib.sha256(self.exe.read_bytes()).hexdigest(),
            )
        )

    def test_complete_identity_is_accepted(self) -> None:
        result = authenticate_extraction(self.root)
        self.assertEqual(result.executable, self.exe.resolve())
        self.assertEqual(result.sha256, hashlib.sha256(self.exe.read_bytes()).hexdigest())

    def test_modified_executable_is_rejected(self) -> None:
        self.exe.write_bytes(b"Synthetic title fixture")
        with self.assertRaisesRegex(IdentityError, "SHA-256 mismatch"):
            authenticate_extraction(self.root)

    def test_truncated_executable_is_rejected(self) -> None:
        self.exe.write_bytes(b"short")
        with self.assertRaisesRegex(IdentityError, "exactly"):
            authenticate_extraction(self.root)

    def test_oversized_executable_is_rejected(self) -> None:
        self.exe.write_bytes(self.exe.read_bytes() + b"extra")
        with self.assertRaisesRegex(IdentityError, "exactly"):
            authenticate_extraction(self.root)

    def test_configuration_limit_accepts_exact_size_and_rejects_overflow(self) -> None:
        valid = self.cnf.read_bytes()
        self.cnf.write_bytes(valid + b"#" * (4096 - len(valid)))
        self.assertEqual(authenticate_extraction(self.root).executable, self.exe.resolve())
        with self.cnf.open("ab") as output:
            output.write(b"#")
        with self.assertRaisesRegex(IdentityError, "4096-byte configuration bound"):
            authenticate_extraction(self.root)

    def test_non_ascii_configuration_is_rejected(self) -> None:
        self.cnf.write_bytes(self.cnf.read_bytes() + b"\xff")
        with self.assertRaisesRegex(IdentityError, "not ASCII"):
            authenticate_extraction(self.root)

    def test_stale_file_size_cannot_bypass_read_bounds(self) -> None:
        real_stat = Path.stat
        for target, suffix, message in (
            (self.cnf, b"#" * 4096, "configuration bound"),
            (self.exe, b"extra", "exactly"),
        ):
            with self.subTest(file=target.name):
                original = target.read_bytes()
                target.write_bytes(original + suffix)

                def stale_stat(
                    path: Path, *, follow_symlinks: bool = True
                ) -> os.stat_result:
                    metadata = real_stat(path, follow_symlinks=follow_symlinks)
                    if path != target:
                        return metadata
                    fields = list(metadata)
                    fields[6] = len(original)
                    return os.stat_result(fields)

                with patch.object(Path, "stat", stale_stat):
                    with self.assertRaisesRegex(IdentityError, message):
                        authenticate_extraction(self.root)
                target.write_bytes(original)

    def test_boot_name_must_match_even_with_correct_executable(self) -> None:
        self.cnf.write_text("BOOT = cdrom:\\OTHER.EXE;1\n", encoding="ascii")
        with self.assertRaisesRegex(IdentityError, "BOOT entry"):
            authenticate_extraction(self.root)

    def test_ambiguous_boot_is_rejected(self) -> None:
        valid = self.cnf.read_text()
        for duplicate in (valid, "BOOT =\n"):
            with self.subTest(duplicate=duplicate):
                self.cnf.write_text(valid + duplicate, encoding="ascii")
                with self.assertRaisesRegex(IdentityError, "exactly one"):
                    authenticate_extraction(self.root)

    def test_missing_extraction_is_rejected(self) -> None:
        with self.assertRaisesRegex(IdentityError, "missing SYSTEM.CNF"):
            authenticate_extraction(self.root / "absent")


if __name__ == "__main__":
    unittest.main()
