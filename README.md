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

- **Release** builds `MGSDelta_Plugin.dll` straight into `UEVR_profile/MGSDelta-Win64-Shipping/plugins/`.
  Symlink (or copy) `UEVR_profile/MGSDelta-Win64-Shipping` to
  `%APPDATA%\UnrealVRMod\MGSDelta-Win64-Shipping` to test in-game:

  ```
  mklink /D "%APPDATA%\UnrealVRMod\MGSDelta-Win64-Shipping" "<repo>\UEVR_profile\MGSDelta-Win64-Shipping"
  ```

- **Debug** builds into `build/<preset>/bin/Debug/` and is never deployed automatically.

The output folder can be changed with the `MGSDELTA_PLUGIN_DIR` CMake cache variable.

### Temporary requirement: Dumper-7 SDK

The plugin still uses a Dumper-7 SDK dump of the game, which is not part of the repository.
Place it in `sdk/` (the folder containing `SDK.hpp` and `SDK/`) or point `MGSDELTA_SDK_DIR` to it.
This dependency is being removed in favor of UEVR API reflection calls, so the plugin no longer
needs rebuilding after game updates.

## Repository layout

| Path          | Contents                                                        |
|---------------|-----------------------------------------------------------------|
| `src/`        | Plugin source code                                              |
| `external/`   | Vendored third-party code: UEVR API headers, Dear ImGui + render hooks, magic_enum, nlohmann/json |
| `UEVR_profile/` | The UEVR profile shipped to users (config, Lua scripts, bindings) |
| `build/`      | Generated build files (not committed, safe to delete)          |

Fetched at configure time: [kananlib](https://github.com/cursey/kananlib),
[spdlog](https://github.com/gabime/spdlog), [bddisasm](https://github.com/bitdefender/bddisasm),
[glm](https://github.com/g-truc/glm).
