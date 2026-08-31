"""Prepare C-12: Final Resistance's bare psxport substrate from a user-supplied disc."""
from __future__ import annotations

import argparse
import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent
FRAMEWORK = ROOT / "external" / "psxport"
BUILD = ROOT / "build" / "player"
EXTRACTED = ROOT / "scratch" / "bin" / "c12"
GENERATED_SOURCE = ROOT / "generated" / "recompiled.c"


class Refusal(RuntimeError):
    pass


def run(command: list[str], environment: dict[str, str], *, cwd: Path = ROOT) -> None:
    completed = subprocess.run(command, cwd=cwd, env=environment, check=False)
    if completed.returncode:
        raise Refusal(f"command failed ({completed.returncode}): {' '.join(command)}")


def pin() -> tuple[str, str]:
    values = {}
    for line in (ROOT / "psxport.pin").read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("=")
        if separator:
            values[key.strip()] = value.strip()
    if not values.get("url") or not values.get("commit"):
        raise Refusal("psxport.pin must provide url and immutable commit")
    return values["url"], values["commit"]


def bootstrap_framework(environment: dict[str, str]) -> None:
    if (FRAMEWORK / "cmake" / "psxport.cmake").is_file():
        return
    shared = ROOT.parent / "psxport"
    FRAMEWORK.parent.mkdir(parents=True, exist_ok=True)
    if (shared / "cmake" / "psxport.cmake").is_file():
        FRAMEWORK.symlink_to(os.path.relpath(shared, FRAMEWORK.parent))
        return
    url, commit = pin()
    run(["git", "clone", url, str(FRAMEWORK)], environment)
    run(["git", "checkout", commit], environment, cwd=FRAMEWORK)
    run(["git", "submodule", "update", "--init", "vendor/beetle-psx", "vendor/lucent"], environment, cwd=FRAMEWORK)
    run(["git", "submodule", "update", "--init", "deps/libchdr"], environment, cwd=FRAMEWORK / "vendor" / "beetle-psx")


def ensure_framework(environment: dict[str, str]) -> None:
    bootstrap_framework(environment)
    run([
        sys.executable,
        "-B",
        str(FRAMEWORK / "tools" / "psxport_sync.py"),
        "--consumer",
        str(ROOT),
        "--auto",
    ], environment)


def resolve_disc(value: str | None, environment: dict[str, str]) -> Path:
    candidate = Path(value).expanduser() if value else None
    if candidate is None and environment.get("PSXPORT_DISC"):
        candidate = Path(environment["PSXPORT_DISC"]).expanduser()
    if candidate is None and (ROOT / ".env").is_file():
        for line in (ROOT / ".env").read_text(encoding="utf-8").splitlines():
            key, separator, raw = line.partition("=")
            if separator and key.strip() == "PSXPORT_DISC":
                candidate = Path(raw.strip()).expanduser()
                break
    if candidate is None:
        drops = sorted(ROOT.glob("*.chd"))
        if len(drops) == 1:
            candidate = drops[0]
        elif len(drops) > 1:
            raise Refusal("multiple .chd files found; pass --disc or set PSXPORT_DISC")
    if candidate is None or not candidate.is_file():
        raise Refusal("supply one disc with --disc, PSXPORT_DISC, .env, or a root-level .chd")
    return candidate.resolve()


def extracted_executable() -> Path:
    files = [path for path in EXTRACTED.iterdir() if path.is_file() and path.read_bytes()[:8] == b"PS-X EXE"]
    if len(files) != 1:
        raise Refusal(f"discdump produced {len(files)} PS-X EXE files in {EXTRACTED}; expected one")
    return files[0]


def configure(environment: dict[str, str]) -> None:
    run([
        "cmake", "-S", str(ROOT), "-B", str(BUILD), f"-DPython3_EXECUTABLE={sys.executable}",
        "-DBUILD_TESTING=OFF", "-DPSXPORT_BUILD_TESTS=OFF",
    ], environment)
    run([
        sys.executable,
        "-B",
        str(FRAMEWORK / "tools" / "psxport_sync.py"),
        "--consumer",
        str(ROOT),
        "--check",
        "--resolved",
        str(BUILD / "psxport_resolved.txt"),
    ], environment)


def prepare(disc: Path, environment: dict[str, str]) -> tuple[Path, Path]:
    ensure_framework(environment)
    configure(environment)
    run(["cmake", "--build", str(BUILD), "--target", "discdump", "c12_scaffold", "-j", str(os.cpu_count() or 1)], environment)
    EXTRACTED.mkdir(parents=True, exist_ok=True)
    run([str(BUILD / "psxport_build" / "tools" / "discdump"), str(disc), str(EXTRACTED)], environment)
    executable = extracted_executable()
    GENERATED_SOURCE.parent.mkdir(parents=True, exist_ok=True)
    run([
        sys.executable,
        "-B",
        str(FRAMEWORK / "tools" / "recomp" / "emit.py"),
        str(executable),
        str(GENERATED_SOURCE),
        "--whole-program",
    ], environment)
    configure(environment)
    player = BUILD / "c12_port"
    run(["cmake", "--build", str(BUILD), "--target", "c12_port", "-j", str(os.cpu_count() or 1)], environment)
    if not player.is_file():
        raise Refusal(f"CMake built c12_port but did not produce {player}")
    return executable, player


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--disc", help="user-supplied PSX CHD")
    parser.add_argument("--prepare-only", action="store_true", help="provision and emit without requesting launch")
    args = parser.parse_args(argv)
    environment = dict(os.environ)
    disc = resolve_disc(args.disc, environment)
    environment["PSXPORT_DISC"] = str(disc)
    environment.setdefault("PSXPORT_ASSET_DIR", str(FRAMEWORK))
    executable, player = prepare(disc, environment)
    if args.prepare_only:
        return 0
    os.execvpe(str(player), [str(player), str(executable)], environment)


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Refusal as error:
        print(f"[run] REFUSED: {error}", file=sys.stderr)
        raise SystemExit(2)
