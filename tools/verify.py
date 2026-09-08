"""Verify C-12's redistributable startup boundary through the shared build owner."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FRAMEWORK = ROOT / "external" / "psxport"
if not (FRAMEWORK / "tools" / "port" / "consumer_verify.py").is_file():
    raise SystemExit(f"C-12 verification requires the shared psxport checkout at {FRAMEWORK}")
sys.path.insert(0, str(FRAMEWORK / "tools"))

from port.consumer_verify import ConsumerVerifyConfig, run_consumer_verification


def main() -> int:
    build = ROOT / "build" / "maintainer"
    return run_consumer_verification(
        ConsumerVerifyConfig(
            name="C-12 authenticated startup probe",
            root=ROOT,
            build=build,
            psxport=FRAMEWORK,
            product=build / "c12_boot_probe",
            cmake_module=ROOT / "CMakeLists.txt",
            test_regex="^c12_",
            cmake_definitions=("-DC12_BUILD_RUNTIME=ON", "-DBUILD_TESTING=ON"),
        )
    )


if __name__ == "__main__":
    raise SystemExit(main())
