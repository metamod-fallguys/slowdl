# SlowDL

A standalone [Metamod-FallGuys](https://github.com/hzqst/metamod-fallguys) plugin that monitors GoldSrc UDP (`dlfile`) download requests, including fallback after a failed FastDL request. AngelScript and ASExt are not required.

## Configuration

| Cvar | Default | Effect when nonzero |
| --- | --- | --- |
| `sv_slowdl_kick` | `0` | Disconnect clients requesting ordinary files through UDP |
| `sv_slowdl_console_log` | `1` | Print every client download request to the console |
| `sv_slowdl_file_log` | `1` | Append requests to `<gamedir>/logs/slowdl_YYYYMMDD.log` |

Changes take effect on the next request. Console and file logging are independent. Every record includes the player name, userid, authentication ID, requested file and action. File records also include a timestamp and use the server's local date for the filename. Control characters in client input are escaped. Repeated file write failures produce one diagnostic until writing succeeds again; they do not prevent enforcement.

Two requests are exempt from kicking and are still logged:

- Custom decals: the engine's 36-character `!MD5` request form (case insensitive prefix).
- Sven Co-op's current-map `maps/soundcache/<mapname>.txt`. The mod directory must be named `svencoop`; comparison ignores slash style and ASCII case. Other maps, directory traversal forms and arbitrary files in that directory are not exempt.

The plugin monitors requests, rather than download speed or successful transfers. It does not inspect HTTP traffic. Console-originated commands and requests without exactly one nonempty filename argument are passed to the engine unchanged. A kicked request is logged before disconnect and is not passed to the download handler.

## Installation

Install the complete `addons/` payload under the game's mod directory, including **both** gamedata directories `slowdl/` and `slowdl_windows/`. Add the appropriate line to `addons/metamod/plugins.ini`:

```ini
win32 addons/metamod/dlls/slowdl.dll
linux addons/metamod/dlls/slowdl.so
```

Use this project's Metamod interface 5:17 or newer compatible 5.x. Old BLOB engines additionally require the Metamod revision pinned in `cmake/Revisions.cmake` or a later revision containing BLOB source registration. Metamod loads gamedata at startup, so restart after installing new data. Runtime loading is supported when the required catalog was loaded at startup. Runtime unloading is disabled.

## Supported inputs

| Engine build | Windows Win32 | Linux i386 |
| --- | --- | --- |
| Half-Life 3248, 3266, 3329, 3647 (BLOB) | Yes | — |
| Half-Life 4554, 6153 | Yes | — |
| Half-Life 8684, 10210 | Yes | Yes |
| Sven Co-op 8948, 10257 | Yes | Yes |
| Cry of Fear 5936 | Yes | — |

Support is keyed by the exact engine file CRC from the published GoldSrc_VibeSignatures catalog. Unrecognized, modified or unresolved engines fail to load with a diagnostic. `client_t` remains opaque; the plugin does not depend on private client field offsets.

## Build and test

CMake 3.21+, Git and MSVC Win32 or Linux multilib are required. Native builds download and validate gamedata with Python 3.8+.

```sh
cmake -S . -B build/x86/Release -A Win32
cmake --build build/x86/Release --config Release --parallel 4
ctest --test-dir build/x86/Release -C Release --output-on-failure
cmake --install build/x86/Release --config Release
```

On Linux omit `-A Win32` and add `-DCMAKE_BUILD_TYPE=Release`. Configure Debug in a separate `build/x86/Debug` tree. `METAMOD_SOURCE_PATH` and `FORMAT_VALIDATION_SOURCE_PATH` accept local dependency trees; otherwise pinned revisions are fetched. Default installation is `install/x86/<Config>/addons/`.

Tests exercise ordinary and exempt downloads, all cvar combinations, invalid contexts, log escaping, forwarding/disconnect order, and file write failure recovery. They do not substitute for connection and FastDL integration tests on each engine build.

See [verification results and live-game limits](docs/verification.md).

Formatting uses clang-format 23.1.3:

```sh
cmake -S . -B build/format -DFORMAT_VALIDATION_ONLY=ON
cmake --build build/format --target slowdl-format-check
```
