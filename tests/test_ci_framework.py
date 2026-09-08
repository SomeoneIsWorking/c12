from __future__ import annotations

import tempfile
import unittest
from collections.abc import Sequence
from pathlib import Path

from tools.ci_framework import bootstrap

ROOT = Path(__file__).resolve().parents[1]


class FrameworkBootstrapTests(unittest.TestCase):
    def setUp(self) -> None:
        scratch = ROOT / "scratch" / "tests"
        scratch.mkdir(parents=True, exist_ok=True)
        self.directory = self.enterContext(tempfile.TemporaryDirectory(dir=scratch))
        self.root = Path(self.directory)
        self.revision = "a" * 40
        self.actual = self.revision
        self.dirty = ""
        self.commands: list[list[str]] = []
        (self.root / "psxport.pin").write_text(
            f"url = https://example.invalid/psxport.git\ncommit = {self.revision}\n",
            encoding="utf-8",
        )

    def record_git(self, command: Sequence[str], cwd: Path) -> str:
        self.commands.append(list(command))
        if command[1] == "clone":
            Path(command[-1]).mkdir()
        if command[1] == "rev-parse":
            return self.actual
        if command[1] == "status":
            return self.dirty
        return ""

    def test_cold_clone_uses_the_title_pin(self) -> None:
        framework = bootstrap(self.root, self.record_git)
        self.assertEqual(framework, self.root / "external/psxport")
        self.assertEqual(self.commands[1], ["git", "checkout", "--detach", self.revision])
        self.assertEqual(len(self.commands), 4)

    def test_existing_matching_checkout_is_read_only(self) -> None:
        (self.root / "external/psxport").mkdir(parents=True)
        bootstrap(self.root, self.record_git)
        self.assertEqual([command[1] for command in self.commands], ["rev-parse", "status"])

    def test_mismatched_existing_checkout_is_not_overwritten(self) -> None:
        (self.root / "external/psxport").mkdir(parents=True)
        self.actual = "b" * 40
        with self.assertRaisesRegex(RuntimeError, "pin mismatch"):
            bootstrap(self.root, self.record_git)
        self.assertEqual([command[1] for command in self.commands], ["rev-parse"])

    def test_dirty_matching_checkout_refuses(self) -> None:
        (self.root / "external/psxport").mkdir(parents=True)
        self.dirty = " M runtime.cpp"
        with self.assertRaisesRegex(RuntimeError, "checkout has changes"):
            bootstrap(self.root, self.record_git)

    def test_branch_name_is_not_an_immutable_pin(self) -> None:
        (self.root / "psxport.pin").write_text("url = example\ncommit = main\n", encoding="utf-8")
        with self.assertRaisesRegex(RuntimeError, "immutable"):
            bootstrap(self.root, self.record_git)
        self.assertEqual(self.commands, [])


if __name__ == "__main__":
    unittest.main()
