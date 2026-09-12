# C-12 direct runtime had no typed VSync boundary

- status: resolved
- state: S003, S004
- discovered: 2026-09-12
- resolved: 2026-09-12

## Cause

`C12Runtime` supplied the authenticated executable image but returned no `PlatformHlePlan`. The
shared direct-runtime path consequently had no title-owned libetc VSync address to bind, so a future
whole-program Lightrec run could only stop at an unowned guest service or rely on an old static path.

## Resolution

`game/runtime/c12_platform_facts.*` now publishes the recorded C-12 libetc VSync entry at
`0x800A1758` through an exact four-byte admission window. The framework owns the resulting typed
`FrameBoundary` exit; C-12 does not duplicate that service or install a guest-code override. The
`c12_runtime_services` test exercises plan publication, installation, continuation preservation, and
refusal of the adjacent guest address. The diagnostic boot probe now calls `initBuiltins` itself before
guest execution; it had bypassed the product frame-loop preflight that installs the same plan. The
authenticated USA image reached a typed `FrameBoundary` at `0x800A1758` after 302,824 cycles and
resumed 29 subsequent bounded turns with nonzero Lightrec work, zero fallback, and zero faults.

## Remaining gap

This is a runtime-contract milestone, not gameplay or field-loop conformance. The diagnostic repeats
the title's `VSync(-1)` query at caller `0x800B4EBC` / continuation `0x800B4EC4` because it has no
native field lifecycle. The product must recover the title startup and frame driver, keep guest VSync
from owning time, and demonstrate execution beyond `0x800A7F90`. The exact linked library body extent
remains unrecovered, so the admission window stays intentionally narrow.
