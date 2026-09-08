from __future__ import annotations

import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from source_policy import SourcePolicyError, check_source_policy


def temporary_directory() -> tempfile.TemporaryDirectory[str]:
    scratch = ROOT / "scratch" / "tests"
    scratch.mkdir(parents=True, exist_ok=True)
    return tempfile.TemporaryDirectory(dir=scratch)


class SourcePolicyTests(unittest.TestCase):
    def test_shipping_tree_passes(self) -> None:
        self.assertGreater(check_source_policy(ROOT), 0)

    def test_generated_directory_fails(self) -> None:
        with temporary_directory() as directory:
            root = Path(directory)
            (root / "generated").mkdir()
            with self.assertRaisesRegex(SourcePolicyError, "retired path"):
                check_source_policy(root)

    def test_static_dispatch_marker_fails(self) -> None:
        with temporary_directory() as directory:
            root = Path(directory)
            (root / "game").mkdir()
            (root / "game" / "owner.cpp").write_text(
                "main_dispatch(core);", encoding="utf-8"
            )
            with self.assertRaisesRegex(
                SourcePolicyError, "forbidden static execution"
            ):
                check_source_policy(root)

    def test_tool_policy_filename_does_not_hide_static_dispatch(self) -> None:
        with temporary_directory() as directory:
            root = Path(directory)
            path = root / "tools" / "other" / "policy.py"
            path.parent.mkdir(parents=True)
            path.write_text("main_dispatch(core)", encoding="utf-8")
            with self.assertRaisesRegex(SourcePolicyError, "tools/other/policy.py"):
                check_source_policy(root)

    def test_direct_product_environment_and_diagnostics_fail(self) -> None:
        with temporary_directory() as directory:
            root = Path(directory)
            (root / "game").mkdir()
            (root / "game" / "owner.cpp").write_text(
                'void bad() { getenv("BAD"); fprintf(stderr, "bad"); }',
                encoding="utf-8",
            )
            with self.assertRaisesRegex(SourcePolicyError, "configurable logger"):
                check_source_policy(root)

    def test_non_launcher_shell_automation_fails(self) -> None:
        with temporary_directory() as directory:
            root = Path(directory)
            (root / "game").mkdir()
            (root / "game" / "owner.cpp").write_text(
                "void owner() {}", encoding="utf-8"
            )
            (root / "helper.sh").write_text("#!/bin/sh\n", encoding="utf-8")
            with self.assertRaisesRegex(SourcePolicyError, "automation must be Python"):
                check_source_policy(root)

    def test_dependency_build_scripts_are_not_title_automation(self) -> None:
        with temporary_directory() as directory:
            root = Path(directory)
            (root / "CMakeLists.txt").write_text("project(fixture)", encoding="utf-8")
            for name in ("build", "external", "scratch"):
                (root / name).mkdir()
                (root / name / "helper.sh").write_text("#!/bin/sh\n", encoding="utf-8")
            self.assertEqual(check_source_policy(root), 1)


if __name__ == "__main__":
    unittest.main()
