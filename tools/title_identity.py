"""C-12 USA revision policy for files extracted by psxport's disc reader."""

from __future__ import annotations

import argparse
import hashlib
import json
import logging
import re
from dataclasses import dataclass
from pathlib import Path

_IDENTITY = json.loads(
    (Path(__file__).resolve().parents[1] / "title.json").read_text(encoding="utf-8")
)
EXECUTABLE_NAME: str = _IDENTITY["executable"]
EXECUTABLE_SHA256: str = _IDENTITY["sha256"]
EXECUTABLE_SIZE: int = _IDENTITY["size"]
CONFIG_SIZE_LIMIT = 4096


class IdentityError(RuntimeError):
    """The supplied files do not identify the supported C-12 USA revision."""


@dataclass(frozen=True)
class AuthenticatedProgram:
    executable: Path
    sha256: str


def authenticate_extraction(directory: Path) -> AuthenticatedProgram:
    """Require both the disc boot selection and the complete known executable bytes."""
    config = directory / "SYSTEM.CNF"
    executable = directory / EXECUTABLE_NAME
    for path in (config, executable):
        if not path.is_file():
            raise IdentityError(f"C-12 extraction is missing {path.name}")
    with config.open("rb") as source:
        config_bytes = source.read(CONFIG_SIZE_LIMIT + 1)
    if len(config_bytes) > CONFIG_SIZE_LIMIT:
        raise IdentityError(
            f"C-12 SYSTEM.CNF exceeds the {CONFIG_SIZE_LIMIT}-byte configuration bound"
        )
    try:
        text = config_bytes.decode("ascii")
    except UnicodeDecodeError as error:
        raise IdentityError("C-12 SYSTEM.CNF is not ASCII text") from error
    entries = re.findall(
        r"^[ \t]*BOOT[ \t]*=[ \t]*([^\r\n]*)",
        text,
        flags=re.MULTILINE | re.IGNORECASE,
    )
    expected = f"cdrom:\\{EXECUTABLE_NAME};1"
    if len(entries) != 1 or entries[0].strip().casefold() != expected.casefold():
        raise IdentityError(
            f"C-12 requires exactly one SYSTEM.CNF BOOT entry selecting {expected}"
        )
    with executable.open("rb") as source:
        executable_bytes = source.read(EXECUTABLE_SIZE + 1)
    if len(executable_bytes) != EXECUTABLE_SIZE:
        raise IdentityError(
            f"C-12 {EXECUTABLE_NAME} must contain exactly {EXECUTABLE_SIZE} bytes"
        )
    digest = hashlib.sha256(executable_bytes).hexdigest()
    if digest != EXECUTABLE_SHA256:
        raise IdentityError(f"C-12 USA executable SHA-256 mismatch: {digest}")
    return AuthenticatedProgram(executable.resolve(), digest)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path, help="directory produced by psxport discdump")
    args = parser.parse_args()
    logging.basicConfig(level=logging.INFO, format="[c12.identity] %(message)s")
    try:
        program = authenticate_extraction(args.directory)
    except (IdentityError, OSError) as error:
        logging.error("REFUSED: %s", error)
        return 2
    logging.info(
        "authenticated C-12 USA %s; SHA-256 %s", program.executable.name, program.sha256
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
