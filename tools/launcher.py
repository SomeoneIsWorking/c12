"""Resolve C-12 player inputs and stop at the missing shared dynarec boundary."""

from __future__ import annotations

import argparse
import logging
import os
from collections.abc import Sequence
from pathlib import Path

from tools.config import ConfigurationError, resolve_config

ROOT = Path(__file__).resolve().parents[1]
LOGGER = logging.getLogger("c12.launcher")
MISSING_RUNTIME = "Lightrec backend and title typed-exit runner are not integrated"


def main(argv: Sequence[str] | None = None) -> int:
    logging.basicConfig(
        level=logging.INFO, format="[%(name)s] %(levelname)s: %(message)s"
    )
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--disc", type=Path, help="user-supplied C-12 CHD")
    parser.add_argument(
        "--check",
        action="store_true",
        help="validate configuration without building or launching",
    )
    args = parser.parse_args(argv)
    try:
        config = resolve_config(ROOT, args.disc, os.environ)
    except ConfigurationError as error:
        LOGGER.error("%s", error)
        return 2
    if args.check:
        LOGGER.info("validated configured disc path: %s", config.disc)
        return 0
    LOGGER.error("C-12 gameplay target unavailable: %s", MISSING_RUNTIME)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
