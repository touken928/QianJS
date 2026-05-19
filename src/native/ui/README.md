# `ui` 模块（可选）

使用 **SDL2** 渲染器在窗口中做 2D 绘制（矩形、折线等）。**SDL2** 以 **Git 子模块** 形式位于 **`third_party/sdl2`**，由 **`cmake/ui.cmake`** 以 **`add_subdirectory`** 静态编入；**不要求**系统预装 `libsdl2-dev` / Homebrew `sdl2` 等开发包。克隆后需 **`git submodule update --init third_party/sdl2`**（或 **`--recurse-submodules`**）。默认 **`QIANJS_MODULE_UI=OFF`**。

## JS API（`import * as ui from 'ui'`）

| 函数 | 说明 |
|------|------|
| `init(width, height, title?)` | 创建窗口与渲染器；坐标系 **y 向下** |
| `close()` | 销毁资源并 `SDL_QuitSubSystem(VIDEO)` |
| `pollEvents()` | 排空 SDL 事件队列；若本批含 **QUIT** 或 **Esc 按下** 返回 `true`（不返回具体事件，与 `readEvents` 二选一或先后调用时后者可能无事件） |
| `readEvents()` | 排空队列并返回 `{ quit: boolean, events: Array }`；`events` 见下文 |
| `getMouseState()` | 当前鼠标相对**焦点窗口**的坐标与按键：`{ x, y, buttons: { left, middle, right, x1, x2 } }` |
| `getModState()` | 修饰键：`{ shift, ctrl, alt, gui, num, caps, lshift, rshift, …, raw }`（`raw` 为 SDL 位掩码） |
| `isKeyDown(nameOrSym)` | 参数为 **`SDL_GetKeyFromName`** 所用字符串（如 `"Space"`、`"Left"`）或 **SDLK_** 整型键码；当前帧键盘状态是否为按下 |
| `present()` | 立即执行当前 **DrawList** 并 `SDL_RenderPresent`（`runApp` 会在每帧 `render()` 后自动 present） |
| `setSourceRGBA(r, g, b, a)` | 当前笔触/填充颜色（分量 0–1） |
| `clear(r, g, b, a)` | 以实色清空整窗（不改变 `setSourceRGBA` 所存笔触色） |
| `fillRect(x, y, w, h)` | 填充矩形 |
| `strokeRect(x, y, w, h)` | 描边矩形 |
| `setLineWidth(w)` | 线宽（`stroke` / `strokeRect` 近似） |
| `moveTo(x, y)` / `lineTo(x, y)` / `stroke()` | 折线描边 |

游戏循环请使用 **`app.runApp(createApp({ update, render }))`**。无窗口 headless：`QIANJS_NULL_UI=1`（见根目录 **`AGENTS.md`**）。

### `readEvents` / `runApp` 的 `input.events` 元素

| `type` | 主要字段 |
|--------|----------|
| `quit` | — |
| `keydown` / `keyup` | `repeat`, `scancode`, `sym`, `mod`, `key`（`SDL_GetKeyName`，如 `"A"`、`"Return"`） |
| `mousemove` | `x`, `y`, `dx`, `dy`, `which` |
| `mousedown` / `mouseup` | `button`（`"left"` / `"middle"` / `"right"` / `"x1"` / `"x2"`）, `buttonId`, `x`, `y`, `clicks`, `which` |
| `mousewheel` | `x`, `y`, `direction`, `which` |

键名与 SDL2 一致，可查 [SDL_Keycode](https://wiki.libsdl.org/SDL2/SDLKeycodeLookup) / `SDL_GetKeyFromName`。

脚本参数需通过 **`argv()`** 读取（见 [`process` 模块说明](../process/README.md)）。
