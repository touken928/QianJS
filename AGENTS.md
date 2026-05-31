# AGENTS.md — QianJS

## Build

```bash
cmake --preset=dev
cmake --build --preset=dev
ctest --preset=dev --output-on-failure
```

- **Submodules**: `qjs`, `libuv`, `uvw`, `sdl2` (window/events). **NanoVG** is vendored under `third_party/nanovg` (not a submodule yet).
- **MSVC unsupported** — use MinGW-w64 or Clang on Windows.

## CMake / module profiles

| Variable | Notes |
|----------|-------|
| `QIANJS_PROFILE` | **SKU**: `minimal` (console, process), `io` (+ timers, fs), `desktop` (+ canvas, game). Empty/`custom` = use `QIANJS_MODULE_*` options. |
| `QIANJS_MODULE_*` | Per-module override; catalog in [`src/native/native_modules.cmake`](src/native/native_modules.cmake). |
| `QIANJS_BUILD_PROFILE` / `QIANJS_BUILD_MODULES` | Generated in `qianjs_modules.h` (observability, `help` text). |

Presets `dev` / `test` / `prod` set `QIANJS_PROFILE=desktop`. `io` / `minimal` presets available for SKU CI.

Module DAG: `canvas` → UI_STACK (SDL + NanoVG); `game` → GAME_STACK (frame loop) + **DEPS** `canvas`; `fs`|`timers` → libuv. Glue (`qianjs_default_plugins.g.cc`) registers plugins in topological order.

## Architecture

```
qianjs_cli_run (src/app/cli.cc)
  ├─ commands/build|embed|run  — toolchain (no RuntimeInstance)
  └─ Application::run_script / run_embedded
       └─ RuntimeInstance (one per process invocation)
       ├─ qjs::Engine + RuntimeContext (host)
       ├─ Scheduler (libuv loop, defer queue, uv_timer)
       └─ CanvasRegistry / PlatformCanvas (per createCanvas window)
```

Native modules use `RuntimeInstance::current()` — **no** process-global `g_ui`, `g_timers`, or defer queue.

Plugins implement `qjs::IPlugin::install(qjs::Context&, qjs::Module&)`; registration via `PluginRegistry::installAll(engine.context(), engine.modules())` (see generated `qianjs_default_plugins.g.cc`).

## Lifecycle

`Created → Initialized → Running → Draining → Shutdown → Destroyed`

| Phase | JS | New timer/fs | defer |
|-------|-----|--------------|-------|
| Running | yes | yes | yes |
| Draining | finish only | no | queued only |
| Shutdown | no | no | dropped (generation bump) |

`shutdown()` order: bump **generation** → reject tracked `qjs::Promise` (`PromiseRegistry`) → plugin hooks (`notify_lifecycle`) → cancel timers / clear defer (`scheduler.shutdown(engine_)`). `qjs::Engine` is RAII (no `initialize()` / `cleanup()`); it is destroyed with `RuntimeInstance`.

## Main loop contract

### CLI (`run_until_idle`)

Repeat until idle:

```
pump_async()      # uv tick + run_deferred
pump_microtasks()
```

### Game (`game.run` → `RuntimeInstance::run_frame_loop`)

Per frame:

```
1. PollPlatform     SDL events
2. pump_async()     libuv + defer (timers, fs → JS)
3. update(dt,input) logic
4. pump_microtasks()
5. render()         ctx.* → DrawList
6. present()        NanoVG replay + SDL_GL_SwapWindow
```

## JS API

**`canvas`** — Web Canvas 2D subset (`createCanvas`, `getContext('2d')`, `setFillStyle`, paths, text, …). Each `createCanvas` opens its own SDL window.

**`game`** — frame loop + keyboard (`game.run`, `game.isKeyDown`). Requires `canvas`.

```javascript
import * as canvas from 'canvas';
import * as game from 'game';

const cvs = canvas.createCanvas(480, 420, { title: 'Game' });
const ctx = cvs.getContext('2d');

game.run(cvs, {
  init() {},
  update(dt, input) {},
  render() {
    ctx.setFillStyle('rgb(0,0,0)');
    ctx.fillRect(0, 0, 480, 420);
  },
  shutdown() {},
}, { maxFrames: 128, fps: 60, fixedStep: 1/60 });
```

Headless: `QIANJS_NULL_UI=1` (record draw commands; `present` no-ops GL).

## Threading

- Main thread: JS, SDL, scheduler, defer consumer.
- libuv thread pool: `fs` only; callbacks **defer** to main thread.
- Timers: `uv_timer_t` (no detached threads).

## Testing

- `qianjs::test::TestRuntime` → `instance.initialize(defaultPlugins())` → `begin_script_execution()` → `run_module` (`engine.evalModule(...).success`) → `rt.drain()` → `run_until_idle()`.

## `third_party/qjs` (embedding layer)

Generic QuickJS C++ bindings — **not** libuv/SDL/QianJS lifecycle. QianJS links `qjs::qjs` only.

**Opaque boundary:** `include/qjs/*.h` must not include `quickjs.h` or mention `JSContext` / `JSValue` / `JSRuntime`. QianJS must not use `context().raw()` or raw QuickJS APIs; use `CallContext`, `Engine::call`, `ObjectBuilder` / `ArrayBuilder`, and `Promise::resolve(Value)` instead.

| Include | Role |
|---------|------|
| `<qjs/engine.h>` | `qjs::Engine` — eval/compile, value factories, `call`, promises |
| `<qjs/context.h>` | `qjs::Context` — `modules()`, `engine()` |
| `<qjs/module.h>` | `qjs::Module` — `func`, `value`, `funcDynamic(CallContext&)` |
| `<qjs/call.h>` | `qjs::CallContext`, `NativeDynamicFunction` |
| `<qjs/object.h>` | `ObjectBuilder`, `ArrayBuilder` |
| `<qjs/plugin.h>` | `qjs::IPlugin`, `qjs::PluginRegistry` |
| `<qjs/promise.h>` | `qjs::Promise` — host tracks `Promise*` (e.g. `PromiseRegistry`) |
| `<qjs/value.h>` | `qjs::Value` — opaque handle |
| `<qjs/qjs.h>` | Umbrella include |

QuickJS C API is only used under `third_party/qjs/src/`.

Host responsibilities stay in QianJS: script files (`Embed::readTextFile` + `evalModule`), libuv defer, timers/fs/canvas/game plugins, `PromiseRegistry` on shutdown.

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
| `src/runtime/promise_registry.*` | Track/reject pending `qjs::Promise*` on shutdown |
| `src/runtime/promise_track.h` | Create/release promises tied to `RuntimeInstance` |
| `src/systems/*` | InputSystem + RenderSystem (used by frame loop) |
| `src/platform/*` | DrawList + SDL window (or null platform via env) |
| `src/native/native_modules.cmake` | Module catalog, profile, glue generation |
| `src/native/native_contract.h` | Module threading/defer contract |
| `src/native/*` | JS bindings |
