# `cmake/` — 第三方库 Target 封装

本目录**只**负责把 **`third_party/`** 里的每个子模块接进主工程，并落成**稳定、可组合的 CMake target**。不在 `third_party` 内改 CMake；封装逻辑**扁平**（无子目录）。

内置 native 模块的选项与生成头逻辑在 **`src/native/native_modules.cmake`**（由根 **`CMakeLists.txt`** `include`），不属于本目录职责。

## 原则（现代 CMake）

- **一个 `third_party` 子仓库 ↔ 本目录下一个 `.cmake` 文件**（必要时再 `ALIAS` 成 `qianjs::…`）。
- **Target 是工程边界**：源码、头文件可见性（`INTERFACE` / `PUBLIC` / `PRIVATE`）、编译选项、链接与传递依赖，都应挂在 target 上。
- **消费方只 `target_link_libraries(… PRIVATE|PUBLIC|INTERFACE …)`**，不写裸的 `include_directories` 指进 `third_party/…`（除非上游 target 未导出接口且无法修改子模块——此时在本仓库用 `INTERFACE` 库包一层，仍不直接改 submodule）。
- 子模块自带的 `CMakeLists.txt` **视为黑盒**，仅通过 `add_subdirectory` 与 **cache 预设**（仅限该库文档要求的选项）集成。

## 文件与 Target 对照

| 文件 | `third_party` | 对外使用的 Target |
|------|---------------|-------------------|
| `qjs.cmake` | `qjs`（内含 QuickJS FetchContent） | `qjs::qjs`、`qjs::quickjs`（上游命名，保持不变） |
| `libuv.cmake` | `libuv` | `qianjs::libuv` → `ALIAS` `uv_a` |
| `uvw.cmake` | `uvw` | `qianjs::uvw`：`INTERFACE` 库再 `ALIAS`（因上游 `uvw::uvw` 已是 `ALIAS`，CMake 禁止链式 `ALIAS`） |
| `ui.cmake` | `sdl2` | `qianjs::ui_deps`：`INTERFACE`，链 **SDL2::SDL2-static**（或 **SDL2::SDL2**）及可选 **SDL2::SDL2main**（`add_subdirectory(third_party/sdl2)`；仅 `QIANJS_MODULE_UI=ON` 时由根 `CMakeLists.txt` `include` 并链到 `qianjs_impl`） |

可执行文件 **`qianjs`** 链接静态库 **`qianjs_impl`**（CLI + **`event_loop`** + 启用的 native 模块）；**`qjs::qjs`**、**`Threads`**、按需 **`libuv`/`uvw`** 挂在 **`qianjs_impl`** 上。其它 **`cmake/*.cmake`** 可按模块 **`if(QIANJS_MODULE_…)`** **`include`**，并仅在需要时 **`target_link_libraries(qianjs …)`**。例外：**`libuv.cmake`** / **`uvw.cmake`** 在 **`QIANJS_BUILD_CLI`** 时**总是** **`include`**（运行时固定依赖第三方 target）；**`qianjs`** 是否链接它们、是否定义 **`QIANJS_HAVE_LIBUV`** 仍由 **`fs`** 决定。脚本带幂等保护。内置模块源码与胶水生成见 **`src/native/`**（**`CMakeLists.txt`**、**`native_modules.cmake`**）。约定说明见 **`src/native/README.md`**。

## 新增第三方库时

1. 在 `third_party/` 加入子模块（或目录），**不要改**其内部 CMake（除非上游合并，本仓库 policy 仍建议只在本目录封装）。
2. 在 `cmake/` 新增**一个** `something.cmake`：`add_subdirectory` + 必要 cache 预设 + `ALIAS`/`INTERFACE` 目标。
3. 根 **`CMakeLists.txt`**：常见做法是仅在相关模块**开启**时 **`if(QIANJS_MODULE_…)`** **`include(cmake/something.cmake)`**，并对 **`qianjs`** **`target_link_libraries(PRIVATE …)`**；模块关闭则不 **`include`**、不链接。多模块共用时条件写 **`OR`**，封装脚本建议幂等。若某库像 **libuv/uvw** 一样属 CLI 固定栈，可**无条件** **`include`**，仅用 **`target_link_libraries`** / **`compile_definitions`** 与模块选项挂钩（参见 **`QIANJS_HAVE_LIBUV`**）。
