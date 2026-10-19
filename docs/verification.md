# Issue #34 verification

Recorded on 2026-10-20. Additional live-game testing was stopped at the user's request.

## Automated checks

| Target | Debug | Release |
| --- | --- | --- |
| Windows Win32 aggregate build | Passed | Passed |
| Windows Metamod tests | 9 passed | 9 passed |
| Windows SlowDL behavior test | Passed | Passed |
| Ubuntu 24.04 i386 aggregate build | Passed | Passed |
| Linux Metamod tests (ASan/UBSan) | 16 passed | 16 passed |
| Linux SlowDL behavior test | Passed | Passed |
| Complete installation payload | Passed | Passed |

- Both manifests passed coverage validation for all 15 published platform inputs.
- Standalone Windows Win32 and Linux i386 Release builds fetched the pinned Metamod commit without a local SDK override; builds, behavior tests and installation passed.
- Metamod's gamedata synchronizer tests: 14 passed. Aggregate release helper tests: 42 passed.
- Metamod and SlowDL format checks passed with clang-format 23.1.3.
- The optional Windows `mmfg_test_blob_catalog` is skipped by ordinary CTest without external inputs. Invoked explicitly with the four original BLOB files, it passed in Debug and Release: raw file CRC, description decoding, source registration and five expected symbol addresses per input. The mapped test buffers are not running engines.
- Component tests are run from `<aggregate-build>/metamod/tests` and `<aggregate-build>/slowdl`; the former excludes third-party example tests.

## Live-game evidence and limits

On Windows Sven Co-op 10257, the isolated local server and native client exercised successful HTTP map downloads, HTTP failure followed by UDP map download, ordinary UDP request disconnects, and current-map soundcache requests allowed while kicking was enabled. Logs contained the copied player identity, filename and action. This exposed and fixed the engine-owned cvar copy issue and missing pre-spawn `netname` fallback.

Live-game testing did not cover the other supported builds, real custom decal requests, every independent logging combination, or the final runtime-load Hook correction. The correction selects immediate Hook activation outside startup; it was built and reviewed but was not subsequently exercised in a game. None of these untested cases is recorded as a live-game pass.
