# Code Maestro — UE Bridge

**Work with Unreal Engine using natural language through [Code Maestro](https://www.code-maestro.com/).**

Instead of clicking through the editor and writing glue code yourself, describe what you want and let an agent read your project, navigate Blueprints, and answer questions grounded in your code.

```
"Explain how health regeneration works in this project"
"Open the Blueprint that handles the main menu"
"Show me every class that inherits from AEnemyBase"
```

## Quick Start

**1. Install Code Maestro Desktop**

Download and install [Code Maestro Desktop](https://www.code-maestro.com/). Open it and point it at your UE project.

**2. Install the plugin**

CM Desktop's install wizard fetches the plugin from this repo's latest release, stages it into your project's `Plugins/` directory, and lets UE compile it on next open.

**3. Start talking**

Open your project in the Unreal Editor. The Code Maestro toolbar widget appears in the Level Editor. Connected? You're ready.

## Manual Install

Grab the two assets from the [latest release](https://github.com/codemaestroai/code-maestro-ue-plugin/releases/latest):

1. Extract `CodeMaestroBridge-source.zip` into `<YourProject>/Plugins/CodeMaestroBridge/`
2. Drop `CodeMaestroRuntime.dll` into `<YourProject>/Plugins/CodeMaestroBridge/Binaries/Win64/`
3. Open your project — UE will prompt to build the plugin module the first time

Alternatively, the plugin source is browsable in [`CodeMaestroBridge/`](./CodeMaestroBridge/). You can clone the repo or copy the directory directly into your project's `Plugins/`, then grab `CodeMaestroRuntime.dll` from the latest release.

## What UE Bridge Can Do

**Editor State** — live queries of open assets, selected actors, PIE lifecycle
**Blueprint Navigation** — open Blueprints by name, inspect variables and graphs
**Project Understanding** — grounded answers about your C++ and Blueprint code
**Agent Workflows** — Investigate, Implement, and other agents tuned for UE projects

## Compatibility

| Engine Version | Status |
|---|---|
| UE 5.7 · Windows | Supported |
| UE 5.5 · Windows | Experimental |
| UE 5.4 · Windows | Experimental |
| macOS | Planned |

The plugin is editor-only and does not ship in cooked or packaged builds — no runtime footprint in your shipped game.

## Issues

[Report an issue](https://github.com/codemaestroai/code-maestro-ue-plugin/issues) or reach out through [Code Maestro](https://www.code-maestro.com/).
