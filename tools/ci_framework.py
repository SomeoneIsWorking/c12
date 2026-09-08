"""Fetch C-12's exact psxport pin before its shared CI action is available."""

from __future__ import annotations

import configparser
import logging
import re
import subprocess
from collections.abc import Callable, Sequence
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
Runner = Callable[[Sequence[str], Path], str]


def git(command: Sequence[str], cwd: Path) -> str:
    return subprocess.run(
        command, cwd=cwd, check=True, capture_output=True, text=True
    ).stdout.strip()


def bootstrap(root: Path, runner: Runner = git) -> Path:
    parser = configparser.ConfigParser(interpolation=None)
    parser.read_string("[framework]\n" + (root / "psxport.pin").read_text(encoding="utf-8"))
    pin = parser["framework"]
    url = pin.get("url", "").strip()
    revision = pin.get("commit", "").strip()
    if not url or not re.fullmatch(r"[0-9a-f]{40}", revision):
        raise RuntimeError("psxport.pin requires a URL and an immutable 40-character commit")
    framework = root / "external" / "psxport"
    if not framework.exists() and not framework.is_symlink():
        framework.parent.mkdir(parents=True, exist_ok=True)
        runner(["git", "clone", "--no-checkout", url, str(framework)], root)
        runner(["git", "checkout", "--detach", revision], framework)
    actual = runner(["git", "rev-parse", "HEAD"], framework)
    if actual != revision:
        raise RuntimeError(f"CI framework pin mismatch: expected {revision}, found {actual}")
    dirty = runner(["git", "status", "--porcelain", "--untracked-files=all"], framework)
    if dirty:
        raise RuntimeError("CI requires the exact clean framework pin; existing checkout has changes")
    return framework


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="[c12.ci] %(message)s")
    try:
        framework = bootstrap(ROOT)
    except (OSError, RuntimeError, configparser.Error, subprocess.CalledProcessError) as error:
        logging.error("REFUSED: %s", error)
        return 1
    logging.info("pinned framework ready: %s", framework)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
