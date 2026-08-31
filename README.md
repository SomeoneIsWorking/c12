# C-12: Final Resistance — bare PSX port

This repository is an intentionally minimal psxport consumer. `./run.sh --disc /path/to/disc.chd`
discovers the disc-declared boot executable, extracts it, emits its recompiled substrate, and runs
the psxport generic whole-program profile. It has no title-specific enhancements or native ownership.
The generic profile preserves the generated stack across its discovered VSync boundary; it does not
restart guest main or substitute a title frame loop.
