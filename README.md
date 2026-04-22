# Code Maestro — UE Bridge (Releases)

Release channel for **Code Maestro UE Bridge** — an Unreal Engine 5.7 editor plugin that connects your UE project to [Code Maestro](https://codemaestro.com).

> **This repo contains release artifacts only.** Source code lives in a private development repository. Use the CM Desktop app to install the plugin automatically, or download the release assets below for a manual install.

## Install (recommended)

Install [Code Maestro Desktop](https://codemaestro.com/download) and point it at your UE project. CM Desktop's in-editor wizard fetches the latest release from this repo, stages the plugin into your project's `Plugins/` directory, and lets UE compile it on next open.

## Manual install

1. Download both assets from the [latest release](https://github.com/codemaestroai/code-maestro-ue-plugin/releases/latest):
   - `CodeMaestroBridge-source.zip` — the UE plugin source
   - `CodeMaestroRuntime.dll` — the networking runtime the plugin loads at startup
2. Extract the zip into `<YourProject>/Plugins/CodeMaestroBridge/`
3. Drop `CodeMaestroRuntime.dll` into `<YourProject>/Plugins/CodeMaestroBridge/Binaries/Win64/`
4. Open your UE project; it will prompt to build the plugin module the first time

## Compatibility

| Platform | Status |
|---|---|
| Windows (UE 5.7) | Supported |
| macOS | Planned |
| Linux | Not planned |

The plugin is **editor-only** (`"Type": "Editor"` in the `.uplugin`) — it loads in the Unreal Editor and does not ship in cooked or packaged builds. No runtime footprint in your shipped game.

## Architecture

The plugin is split into two parts:

- **`CodeMaestroBridge`** — a thin UE editor plugin that registers tool callbacks (editor state, Blueprint queries, PIE lifecycle hooks) and dynamically loads the runtime DLL
- **`CodeMaestroRuntime.dll`** — a standalone C++17 shared library that owns WebSocket connection, session management, and tool protocol. No UE dependency

They talk through a small, version-gated C ABI. The runtime DLL can be updated independently of the plugin source, and vice versa.

## Versioning

Release tags are semver (`v0.2.0`). Pre-releases are tagged `vX.Y.Z-test` or `vX.Y.Z-rc1` and are hidden from CM Desktop's auto-update check.

## License

See [LICENSE](LICENSE).

## Links

- [Code Maestro homepage](https://codemaestro.com)
- [Report an issue](https://github.com/codemaestroai/code-maestro-ue-plugin/issues)
