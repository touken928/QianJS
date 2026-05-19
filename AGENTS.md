# AGENTS.md — QianJS

## Build

```bash
cmake --preset=dev
cmake --build --preset=dev
ctest --preset=dev --output-on-failure
```

- **Submodules**: `qjs`, `libuv`, `uvw`, `sdl2` (UI).
- **MSVC unsupported** — use MinGW-w64 or Clang on Windows.

## CMake / module profiles

| Variable | Notes |
|----------|-------|
| `QIANJS_PROFILE` | **SKU**: `minimal` (console, process), `io` (+ timers, fs), `desktop` (+ ui, app). Empty/`custom` = use `QIANJS_MODULE_*` options. |
| `QIANJS_MODULE_*` | Per-module override; catalog in [`src/native/native_modules.cmake`](src/native/native_modules.cmake). |
| `QIANJS_BUILD_PROFILE` / `QIANJS_BUILD_MODULES` | Generated in `qianjs_modules.h` (observability, `help` text). |

Presets `dev` / `test` / `prod` set `QIANJS_PROFILE=desktop`. `io` / `minimal` presets available for SKU CI.

Module DAG: `app` → `ui` → UI_STACK (SDL); `fs`|`timers` → libuv. Glue (`qianjs_default_plugins.g.cc`) registers plugins in topological order.

## Architecture

```
qianjs_cli_run (src/app/cli.cc)
  ├─ commands/build|embed|run  — toolchain (no RuntimeInstance)
  └─ Application::run_script / run_embedded
       └─ RuntimeInstance (one per process invocation)
       ├─ qjs::JSEngine + RuntimeContext (host)
       ├─ Scheduler (libuv loop, defer queue, uv_timer)
       └─ PlatformWindow (optional, per instance)
```

Native modules use `RuntimeInstance::current()` — **no** process-global `g_ui`, `g_timers`, or defer queue.

## Lifecycle

`Created → Initialized → Running → Draining → Shutdown → Destroyed`

| Phase | JS | New timer/fs | defer |
|-------|-----|--------------|-------|
| Running | yes | yes | yes |
| Draining | finish only | no | queued only |
| Shutdown | no | no | dropped (generation bump) |

`shutdown()` order: bump **generation** → reject tracked native promises → plugin hooks (`notify_lifecycle`) → cancel timers / clear defer → `engine.cleanup()`.

## Main loop contract

### CLI (`run_until_idle`)

Repeat until idle:

```
pump_async()      # uv tick + run_deferred
pump_microtasks()
```

### Game (`runApp` → `RuntimeInstance::run_frame_loop`)

Per frame:

```
1. PollPlatform     SDL events
2. pump_async()     libuv + defer (timers, fs → JS)
3. update(dt,input) logic
4. pump_microtasks()
5. render()         ui.* → DrawList
6. present()        execute DrawList + SDL_RenderPresent
```

## JS API

```javascript
import { createApp, runApp } from 'app';
import * as ui from 'ui';

runApp(createApp({
  init() {},
  update(dt, input) {},
  render() { ui.clear(0,0,0,1); /* ... */ },
  shutdown() {},
}), { width: 480, height: 420, title: 'Game', maxFrames: 128, fps: 60, fixedStep: 1/60 });
```

`createApp` alone registers a deferred app; `Application::run_script` runs it after the script if `runApp` was not called. Pass a positive integer as the last `argv` entry to cap frames (same as examples).

Headless UI (no SDL window): `QIANJS_NULL_UI=1` (DrawList still records; `present` clears without rendering).

## Threading

- Main thread: JS, SDL, scheduler, defer consumer.
- libuv thread pool: `fs` only; callbacks **defer** to main thread.
- Timers: `uv_timer_t` (no detached threads).

## Testing

- `qianjs::test::TestRuntime` → `instance.begin_script_execution()` → `run_module` → `rt.drain()` → `run_until_idle()`.

## Code layout

| Path | Role |
|------|------|
| `src/app/main.cc` | `qianjs` executable entry |
| `src/app/cli.*` | argv dispatch → `qianjs_cli_run` |
| `src/app/application.*` | Script run host (`run_script` / `run_embedded`) |
| `src/app/commands/*` | `build` / `embed` / `run` subcommands |
| `src/runtime/instance.*` | Instance owner |
| `src/runtime/scheduler.*` | libuv + timers + defer |
| `src/runtime/frame_loop.*` | Game frame loop |
| `src/runtime/promise_registry.*` | Track/reject pending native promises on shutdown |
| `src/runtime/script_vm.h` | Thin facade over `RuntimeInstance` |
| `src/systems/*` | InputSystem + RenderSystem (used by frame loop) |
| `src/platform/*` | DrawList + SDL window (or null platform via env) |
| `src/native/native_modules.cmake` | Module catalog, profile, glue generation |
| `src/native/native_contract.h` | Module threading/defer contract |
| `src/native/*` | JS bindings |
