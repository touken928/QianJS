<h1 align="center">QianJS</h1>

<p align="center">
  <strong>轻量、可嵌入、可裁剪的 JavaScript 运行时：支持 ES 模块与字节码运行、脚本编译与嵌入分发，并内置 <code>console</code>、<code>process</code>、<code>timers</code>、<code>fs</code> 等常用模块。</strong>
</p>

<p align="center">
  <a href="https://en.cppreference.com/w/cpp/17"><img src="https://img.shields.io/badge/c++-17-blue.svg?style=for-the-badge&logo=c%2B%2B" alt="C++17"></a>
  <a href="https://cmake.org/"><img src="https://img.shields.io/badge/cmake-3.16+-064F8C.svg?style=for-the-badge&logo=cmake" alt="CMake 3.16+"></a>
  <a href="https://github.com/touken928/qianjs/actions/workflows/ci.yml"><img src="https://img.shields.io/github/actions/workflow/status/touken928/qianjs/ci.yml?style=for-the-badge&logo=githubactions&label=CI" alt="CI"></a>
  <a href="https://github.com/touken928/qianjs/stargazers"><img src="https://img.shields.io/github/stars/touken928/qianjs?style=for-the-badge&color=yellow&logo=github" alt="GitHub stars"></a>
</p>

---

## 项目定位

QianJS 面向“可嵌入、可裁剪”的运行时场景，提供：

- `qianjs run`：运行 `.js` 或 `.qbc`
- `qianjs build`：把 JS 编译到 `./dist/<name>.qbc`
- `qianjs embed`：把字节码附加到可执行文件副本，生成独立程序
- 原生模块：`console`、`process`、`timers`、`fs` / `fs.sync`
- CMake 集成：可直接链接 `qjs::qjs`，不必构建 CLI

---

## 快速开始

### 1) 克隆

```bash
git clone --recurse-submodules https://github.com/touken928/qianjs.git
cd qianjs
```

若克隆时未带子模块：

```bash
git submodule update --init --recursive
```

### 2) 构建

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

默认可执行文件路径通常为 `build/bin/qianjs`（具体随生成器可能不同）。

### 3) 运行示例

```javascript
// main.js
import { log } from 'console';
log('Hello, QianJS!');
```

```bash
qianjs run main.js
```

> 首次配置需要联网，`third_party/qjs` 会通过 FetchContent 拉取 QuickJS。

---

## 命令行用法

```bash
qianjs help

qianjs run main.js                # 运行 ES 模块
qianjs run app.qbc                # 运行字节码
qianjs run main.js arg1 arg2      # 透传脚本参数（process.argv）

qianjs build main.js              # 输出 ./dist/main.qbc
qianjs embed dist/main.qbc        # 生成可独立运行的可执行文件副本
```

### 嵌入执行规则

无参数启动时，`qianjs` 会按顺序尝试：

1. 可执行文件尾部魔数为 `QIANJSBC` 的嵌入字节码
2. 可执行文件同目录下 `<exe名>.qbc`
3. 当前目录下 `<exe名>.qbc`

---

## CMake 选项

| 选项 | 默认 | 说明 |
|------|------|------|
| `QIANJS_BUILD_CLI` | `ON` | 构建 `qianjs` 可执行文件；开启时会纳入 `libuv/uvw` 相关配置 |
| `QIANJS_BUILD_TESTS` | `ON` | 构建 `qianjs_tests` |
| `QIANJS_MODULE_CONSOLE` | `ON` | 启用 `console` |
| `QIANJS_MODULE_PROCESS` | `ON` | 启用 `process` |
| `QIANJS_MODULE_TIMERS` | `ON` | 启用 `timers` |
| `QIANJS_MODULE_FS` | `ON` | 启用 `fs` / `fs.sync` |
| `QIANJS_MODULE_UI` | `OFF` | 启用 `ui`（**SDL2** 来自子模块 **`third_party/sdl2`**，静态链入） |

说明：

- 当 `QIANJS_MODULE_FS=OFF` 时，`qianjs` 不链接 `libuv/uvw`，`QIANJS_HAVE_LIBUV` 为假。
- 当 `QIANJS_MODULE_UI=ON` 时，需已初始化子模块 **`third_party/sdl2`**（`git clone --recurse-submodules` 或 `git submodule update --init third_party/sdl2`）。不依赖系统 `libsdl2-dev` 等开发包。无头/CI 可设 `SDL_VIDEODRIVER=dummy`，并用 `qianjs run examples/breakout.js 2000`、`qianjs run examples/snake.js 3000`、`qianjs run examples/gomoku.js 5000` 等形式传入正整数帧数上限以便脚本自动结束。
- 自动生成头文件在 `${CMAKE_BINARY_DIR}/generated/` 下：`qianjs_modules.h`、`qianjs_default_plugins.g.h`（请勿手改）。

---

## 在其他 CMake 工程中使用

如果你只想用 `qjs::qjs`（不需要 CLI）：

```cmake
set(QIANJS_BUILD_CLI OFF CACHE BOOL "" FORCE)
set(QIANJS_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(path/to/qianjs qianjs_build)

add_executable(myapp main.cc)
target_link_libraries(myapp PRIVATE qjs::qjs)
```

公开 API 头文件位于 `third_party/qjs/include`，核心入口如 `#include <js_engine.h>`，命名空间为 `qjs::`。

---

## 测试

启用 **`QIANJS_BUILD_TESTS`**（默认开启）时通过 **FetchContent** 获取 GoogleTest；首次 **`cmake -B`** 需联网。

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j8
ctest --test-dir build --output-on-failure
```

关闭测试可用：`-DQIANJS_BUILD_TESTS=OFF`。

**CI**： [`.github/workflows/ci.yml`](.github/workflows/ci.yml)

---

## 模块文档

- [`console`](src/native/console/README.md)
- [`process`](src/native/process/README.md)
- [`timers`](src/native/timers/README.md)
- [`fs`](src/native/fs/README.md)
- [`ui`](src/native/ui/README.md)（可选模块，默认关闭）

模块 CMake 接线和目录规范：[`src/native/README.md`](src/native/README.md)。

弹球打砖块：`examples/breakout.js`；贪吃蛇：`examples/snake.js`；双人五子棋：`examples/gomoku.js`（均需构建时打开 `QIANJS_MODULE_UI`）。

---

## 仓库结构

| 路径 | 说明 |
|------|------|
| `cmake/` | 第三方依赖封装（`qjs`、`libuv`、`uvw`、`ui`→SDL2 等） |
| `src/cli/` | CLI 入口 |
| `src/runtime/` | 脚本宿主、事件循环、嵌入辅助 |
| `src/native/` | 内置 native 模块与自动胶水生成 |
| `tests/` | `qianjs_tests`（目录布局对齐 `src/`，见 [`tests/README.md`](tests/README.md)） |
| `third_party/qjs` | qjs 子模块（封装与 QuickJS 拉取逻辑） |
| `third_party/sdl2` | SDL2 源码子模块（仅 `QIANJS_MODULE_UI=ON` 时由 `cmake/ui.cmake` 编入） |

更多构建策略见：[`cmake/README.md`](cmake/README.md)。

---

## 依赖

| 组件 | 来源 |
|------|------|
| QuickJS | 由 `third_party/qjs` 通过 FetchContent 从 [bellard/quickjs](https://github.com/bellard/quickjs) 获取 |
| qjs | 子模块 [touken928/qjs](https://github.com/touken928/qjs) |
| libuv / uvw | 子模块 |
| GoogleTest | CMake **FetchContent**，写在根 **`CMakeLists.txt`**（`QIANJS_BUILD_TESTS=ON` 时；首次 configure 需联网） |
| SDL2 | 子模块 **`third_party/sdl2`**（[libsdl-org/SDL](https://github.com/libsdl-org/SDL)）；仅 **`QIANJS_MODULE_UI=ON`** 时由 **`cmake/ui.cmake`** `add_subdirectory` 静态编译 |

---

## 许可证

QianJS 原创代码采用 [GNU General Public License v3.0](https://www.gnu.org/licenses/gpl-3.0.html)（全文见 [`LICENSE`](LICENSE)）。

第三方依赖保留各自许可证（见 `third_party/**/LICENSE` 或上游说明）。QuickJS 源码遵循 [bellard/quickjs](https://github.com/bellard/quickjs) 的许可；`third_party/qjs` 子模块封装代码许可见 [`third_party/qjs/LICENSE`](third_party/qjs/LICENSE)。
