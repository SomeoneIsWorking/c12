"""Resolve the C-12 disc, extract and authenticate its executable, and launch the product."""

from __future__ import annotations

import argparse
import logging
import os
import subprocess
from collections.abc import Sequence
from pathlib import Path

from tools import title_identity
from tools.config import DISC_VARIABLE, ConfigurationError, resolve_config

ROOT = Path(__file__).resolve().parents[1]
LOGGER = logging.getLogger("c12.launcher")
EXTRACTED = ROOT / "scratch" / "c12-identity"
DISCDUMP = ROOT / "build" / "maintainer" / "framework" / "tools" / "discdump"
PRODUCT = ROOT / "build" / "maintainer" / "c12_port"
BOOT_NAME_VARIABLE = "PSXPORT_C12_EXECUTABLE"


class ProvisioningError(RuntimeError):
    """The available inputs cannot support the authenticated product."""


def _discdump(*args: str) -> None:
    if not DISCDUMP.is_file():
        raise ProvisioningError(
            f"psxport's disc reader is missing: {DISCDUMP}. Build the framework tools first."
        )
    try:
        completed = subprocess.run(
            [str(DISCDUMP), *args], capture_output=True, text=True, check=False, timeout=900
        )
    except OSError as error:
        raise ProvisioningError(f"could not run the disc reader: {error}") from error
    if completed.returncode != 0:
        raise ProvisioningError(
            f"disc reader failed ({completed.returncode}): "
            f"{completed.stderr.strip() or completed.stdout.strip()}"
        )


def provision(disc: Path) -> Path:
    """Extract the executable the disc's SYSTEM.CNF boots and authenticate it."""
    EXTRACTED.mkdir(parents=True, exist_ok=True)
    _discdump("get", "SYSTEM.CNF", str(disc), str(EXTRACTED))
    _discdump("get", title_identity.EXECUTABLE_NAME, str(disc), str(EXTRACTED))
    try:
        program = title_identity.authenticate_extraction(EXTRACTED)
    except title_identity.IdentityError as error:
        raise ProvisioningError(str(error)) from error
    LOGGER.info(
        "provisioned %s (%d bytes, authenticated)",
        program.executable,
        program.executable.stat().st_size,
    )
    return program.executable


def main(argv: Sequence[str] | None = None) -> int:
    logging.basicConfig(level=logging.INFO, format="[%(name)s] %(levelname)s: %(message)s")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--disc", type=Path, help="user-supplied C-12 CHD")
    parser.add_argument(
        "--check",
        action="store_true",
        help="validate configuration and authenticate the disc's executable without launching",
    )
    args = parser.parse_args(argv)
    try:
        config = resolve_config(ROOT, args.disc, os.environ)
        executable = provision(config.disc)
    except (ConfigurationError, ProvisioningError) as error:
        LOGGER.error("%s", error)
        return 2
    if args.check:
        LOGGER.info("validated %s -> %s", config.disc, executable)
        return 0
    if not PRODUCT.is_file():
        LOGGER.error("the C-12 product is not built: %s", PRODUCT)
        return 2
    environment = dict(os.environ)
    environment[BOOT_NAME_VARIABLE] = str(executable)
    environment.setdefault(DISC_VARIABLE, str(config.disc))
    try:
        return subprocess.run([str(PRODUCT)], env=environment, check=False).returncode
    except OSError as error:
        LOGGER.error("could not start the product: %s", error)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())