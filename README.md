# MGSDelta-UEVR

UEVR plugin and profile adding VR improvements and motion controls to **METAL GEAR SOLID Δ: SNAKE EATER**.

## Install

1. Use the latest **nightly** UEVR build: https://github.com/praydog/UEVR-nightly/releases/latest
2. In UEVR, click **Import Config** and select the release zip.
3. If you already had a UEVR profile for this game, delete it first: it may conflict with this one.

## Building from source

### Requirements

- Windows x64
- Visual Studio 2022 or 2026 with the **Desktop development with C++** workload
- CMake 3.24+ (VS 2026 generator needs CMake 4.2+)
- Git (dependencies are downloaded automatically at configure time)

### Build

From Visual Studio: **File → Open → Folder…**, pick the repo folder, select the `vs2022` (or `vs2026`)
preset and build.

From a terminal:

```
cmake --preset vs2022
cmake --build --preset vs2022-release
```

Use `vs2026` / `vs2026-release` for Visual Studio 2026. The generated solution is
`build/vs2022/mgsdelta_uevr.sln` (or `build/vs2026/mgsdelta_uevr.slnx`).

### Output

Every build (Debug or Release) copies `MGSDelta_Plugin.dll` and its `.pdb` into
`UEVR_profile/MGSDelta-Win64-Shipping/plugins/`, so the last configuration built is the one UEVR loads.
Per-configuration binaries stay in `build/<preset>/bin/<Config>/`.

Symlink (or copy) the profile folder into UEVR's profiles folder to test in-game:

```
mklink /D "%APPDATA%\UnrealVRMod\MGSDelta-Win64-Shipping" "<repo>\UEVR_profile\MGSDelta-Win64-Shipping"
```

The deploy folder can be changed with the `MGSDELTA_PLUGIN_DIR` CMake cache variable.
Close the game before building: the copy fails while UEVR has the DLL loaded.

### Game updates

The plugin doesn't embed any game SDK: it finds the game's classes, properties and functions
by name at runtime through the UEVR API (see `src/ue/`), so it usually keeps working after game
updates without a rebuild. If an update renames or changes something the plugin uses, it logs a
`[MGSDelta] ...` error in UEVR's `log.txt` naming exactly what didn't match, and skips that action
instead of crashing.

## Repository layout

| Path          | Contents                                                        |
|---------------|-----------------------------------------------------------------|
| `src/`        | Plugin source code                                              |
| `src/ue/`     | Runtime reflection layer over the UEVR API (properties, function calls, engine types) |
| `external/`   | Vendored third-party code: UEVR API headers, Dear ImGui + render hooks, magic_enum, nlohmann/json |
| `UEVR_profile/` | The UEVR profile shipped to users (config, Lua scripts, bindings) |
| `build/`      | Generated build files (not committed, safe to delete)          |

Fetched at configure time: [kananlib](https://github.com/cursey/kananlib),
[spdlog](https://github.com/gabime/spdlog), [bddisasm](https://github.com/bitdefender/bddisasm),
[glm](https://github.com/g-truc/glm).

## Credits & Special thanks to :
Praydog for the UEVR injector
Jbusfield and his UEVR lua library used for the animated hands.
DJ for all the UEVR modding shared knowledge, for the reverse engineering of the fire function and help digging the sdk dump.
LetMeIn50 for playtesting.
