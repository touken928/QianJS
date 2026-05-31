# Built-in native modules

Each subdirectory (`console/`, `fs/`, …) is one **QuickJS plugin** (`*Plugin` + `*_module.cc`).

## CMake (single catalog)

All registration lives in **[`native_modules.cmake`](native_modules.cmake)**:

| Mechanism | Purpose |
|-----------|---------|
| `qianjs_module(name …)` | Declares CLASS, HEADER, SOURCES, optional `DEPS`, `REQUIRES` |
| `QIANJS_PROFILE` | **SKU**: `minimal` \| `io` \| `desktop` \| `custom` (empty = custom) |
| `QIANJS_MODULE_<NAME>` | Per-module override when profile is `custom` or empty |
| `qianjs_native_attach(target)` | Adds sources, UI stack, writes `build/generated/*.{h,cc}` |

### Profiles

| Profile | Modules |
|---------|---------|
| `minimal` | console, process |
| `io` | + timers, fs |
| `desktop` | + canvas, game |

`fs` / `timers` **REQUIRES** `LIBUV`. `canvas` **REQUIRES** `UI_STACK` (SDL + NanoVG + `src/platform/`). `game` **DEPS** `canvas`, **REQUIRES** `GAME_STACK` (`frame_loop`, input/render systems); `game.run` / `game.isKeyDown`.

### Generated glue (do not edit)

- `qianjs_modules.h` — `QIANJS_MODULE_*`, `QIANJS_BUILD_PROFILE`, `QIANJS_BUILD_MODULES`
- `qianjs_default_plugins.g.h` — declares `qianjs_populate_default_plugins()`
- `qianjs_default_plugins.g.cc` — plugin registration in topological order (linked into `qianjs_impl`)

### Adding a module

1. Implement `src/native/foo/foo_module.{h,cc}` (`qjs::IPlugin`).
2. Append one `qianjs_module(foo …)` block in **`native_modules.cmake`** only.
3. Add `tests/native/foo_module_test.cc` with `#if QIANJS_MODULE_FOO`.
4. Document JS API in `foo/README.md`.

No edits to [`CMakeLists.txt`](CMakeLists.txt) (only calls `qianjs_native_attach`).

## C++ contract

See [`native_contract.h`](native_contract.h): script thread, `defer` for async, no cross-module includes.

## Usage from C++

```cpp
#include "native/default_plugins.h"
// or #include <qianjs_default_plugins.g.h>
auto reg = defaultPlugins();
```

Third-party embedders may build a custom `PluginRegistry` without all modules.
