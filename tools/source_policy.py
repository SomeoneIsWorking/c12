"""C-12 break-first source-boundary checks."""

from __future__ import annotations

import re
from pathlib import Path

RETIRED_PATHS = ("generated", "game/app/main.cpp")
FORBIDDEN_MARKERS = (
    "tools/recomp/emit.py",
    "rec_sources.cmake",
    "main_dispatch",
    "psxport_install_recomp",
    "recomp_iface.h",
    "overlay_table.h",
)
SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx", ".h", ".hpp"})
DIRECT_DIAGNOSTICS = re.compile(
    r"(?:fprintf\s*\(\s*stderr|std::c(?:err|log)\b|OutputDebugString|SDL_Log)"
)
DIRECT_ENVIRONMENT = re.compile(r"\b(?:std::)?getenv\s*\(")


class SourcePolicyError(RuntimeError):
    """A retired static execution path remains reachable."""


def check_source_policy(root: Path) -> int:
    root = root.resolve()
    stale = [relative for relative in RETIRED_PATHS if (root / relative).exists()]
    if stale:
        raise SourcePolicyError("retired path still exists: " + ", ".join(stale))
    candidates = [root / "CMakeLists.txt", root / "bootstrap.py"]
    candidates.extend((root / "game").rglob("*") if (root / "game").is_dir() else ())
    candidates.extend(
        path
        for path in (root / "tools").rglob("*.py")
        if path.name != "source_policy.py"
    )
    paths = sorted(path for path in candidates if path.is_file())
    if not paths:
        raise SourcePolicyError(f"scanned {root} but found 0 source files")
    violations = []
    for path in paths:
        text = path.read_text(encoding="utf-8")
        violations.extend(
            f"{path.relative_to(root)}: {marker}"
            for marker in FORBIDDEN_MARKERS
            if marker in text
        )
        if path.suffix in SOURCE_SUFFIXES and DIRECT_DIAGNOSTICS.search(text):
            violations.append(f"{path.relative_to(root)}: bypasses configurable logger")
        if path.suffix in SOURCE_SUFFIXES and DIRECT_ENVIRONMENT.search(text):
            violations.append(f"{path.relative_to(root)}: bypasses configuration owner")
    violations.extend(
        f"{path.relative_to(root)}: project automation must be Python"
        for path in root.rglob("*.sh")
        if path.is_file() and path.relative_to(root) != Path("run.sh")
    )
    if violations:
        raise SourcePolicyError(
            "forbidden static execution surface:\n" + "\n".join(violations)
        )
    return len(paths)
