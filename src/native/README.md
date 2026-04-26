# 内置 native 模块（目录约定）

本目录每个子文件夹（如 `console/`、`fs/`、`process/`、`timers/`）对应一个 **QuickJS 插件**：在 C++ 里实现 `qjs::JSEngine` 的扩展，对 JS 暴露为内置模块。

最小可用宿主通常保留 **`console` + `process` + `timers`**（日志、argv/env/退出码与基础定时器）；需要文件 I/O 时再开启 **`fs`**。

## 管理（CMake）

| 步骤 | 位置 |
|------|------|
| 声明 **`QIANJS_MODULE_<NAME>`**、登记插件类与头文件路径 | **`src/native/native_modules.cmake`** 里 **`qianjs_native_register_module(<name> <PluginClass> native/<name>/<header>.h [默认 ON 或 OFF])`**（路径相对 **`src/`**；第四参数可选，为模块 **option** 默认值，省略时为 **`ON`**） |
| 把 **`.cc`** 编进 **`qianjs`** | **`src/native/CMakeLists.txt`** 里 **`if(QIANJS_MODULE_<NAME>)` `target_sources(...)`** |

### 第三方库：仅在某模块开启时编译并链接

与 **`qianjs_native_register_module`** 没有隐式耦合；要让「开模块才拉库、关模块完全不参与」统一走下面约定：

1. 在 **`third_party/`** 放依赖源码/子模块，在 **`cmake/`** 增加 **`xxx.cmake`**：`add_subdirectory`（或 FetchContent 等）、导出 **`qianjs::…`**（或稳定别名）。若多个模块可能共用同一库，脚本里对 **`add_subdirectory`** 做幂等（例如 **`if(TARGET qianjs::foo) return()`**），避免重复 **`include`** 报错。
2. 在**根** **`CMakeLists.txt`** 里，用 **`if(QIANJS_MODULE_…)`**（多模块共用时 **`OR`** 组合）再 **`include(cmake/xxx.cmake)`**，并对 **`qianjs`** **`target_link_libraries(PRIVATE …)`**。不走进该分支则不会 **`include`**，一般不会配置/编译该第三方，也不会把其 target 链进 **`qianjs`**。
3. 若运行时或公共代码需要 **`#ifdef`**，再在根里给 **`qianjs`** 加 **`target_compile_definitions`**，条件与第 2 步保持一致。

**当前示例（libuv / uvw）：** 根 **`CMakeLists.txt`** 在 **`QIANJS_BUILD_CLI`** 时**始终** **`include(cmake/libuv.cmake)`**、**`include(cmake/uvw.cmake)`**，第三方 target 始终进入工程。仅当 **`fs`** 开启时 **`qianjs`** 才 **链接** **`qianjs::libuv`** / **`qianjs::uvw`**，并定义 **`QIANJS_HAVE_LIBUV`**（**`event_loop`** 是否用 uv 由该宏决定）。这与第 2 步「按模块 `include`」的通用做法不同，属于**运行时核心栈**的固定集成方式。

配置阶段会在 **`build/generated/`**（或当前 binary dir 下 **`generated/`**）写出 **`qianjs_modules.h`**、**`qianjs_default_plugins.g.h`**（勿手改）。关闭模块示例：`-DQIANJS_MODULE_FS=OFF`。

## 使用（C++）

- **`default_plugins.h`**：`defaultPlugins()` 返回带齐（按 CMake 选项启用的）内置插件的 **`qjs::PluginRegistry`**；**`main.cc`**、**`script_host.h`** 通过它加载默认插件。
- 嵌入场景可自建 **`PluginRegistry`**，不必包含 **`default_plugins.h`**；若仍要复用生成逻辑，可直接 **`#include <qianjs_default_plugins.g.h>`** 并调用 **`qianjs_populate_default_plugins(r)`**（需与 **`qianjs_modules.h`** 中的宏一致）。

## 各模块文档

- [console](console/README.md)
- [fs](fs/README.md)
- [process](process/README.md)
- [timers](timers/README.md)
- [ui](ui/README.md)（默认 **OFF**，子模块 **`third_party/sdl2`**）
