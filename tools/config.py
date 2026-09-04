"""Typed launcher configuration resolved at the process boundary."""

from __future__ import annotations

from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path

DISC_VARIABLE = "PSXPORT_DISC"


class ConfigurationError(RuntimeError):
    """The launcher configuration is incomplete or ambiguous."""


@dataclass(frozen=True)
class LaunchConfig:
    disc: Path


def _read_env_file(path: Path) -> dict[str, str]:
    if not path.is_file():
        return {}
    values: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("=")
        if separator and key.strip() and not key.lstrip().startswith("#"):
            values[key.strip()] = value.strip()
    return values


def resolve_config(
    root: Path, explicit_disc: Path | None, environ: Mapping[str, str]
) -> LaunchConfig:
    configured = environ.get(DISC_VARIABLE) or _read_env_file(root / ".env").get(
        DISC_VARIABLE
    )
    candidate = explicit_disc.expanduser() if explicit_disc else None
    if candidate is None and configured:
        candidate = Path(configured).expanduser()
    if candidate is None:
        drop_ins = sorted(root.glob("*.chd"))
        if len(drop_ins) > 1:
            raise ConfigurationError(
                "multiple root-level .chd files found; pass --disc or set PSXPORT_DISC"
            )
        candidate = drop_ins[0] if drop_ins else None
    if candidate is None or not candidate.is_file():
        raise ConfigurationError(
            "supply the C-12 disc with --disc, PSXPORT_DISC, .env, or one root-level .chd"
        )
    return LaunchConfig(disc=candidate.resolve())
