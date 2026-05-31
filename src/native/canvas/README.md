# canvas

Web Canvas 2D **子集**（NanoVG + SDL 窗口）。`createCanvas` 会立即创建独立窗口；`getContext('2d')` 返回绘制上下文。

| 方法 | 说明 |
|------|------|
| `canvas.pumpEvents()` | 排空 SDL 队列并分发给各窗口；返回 `{ quit }`（应用退出） |
| `canvasElement.present()` | 将当前 2D 命令刷新到窗口 |
| `canvasElement.pollEvents()` | 取本窗口待处理事件 `{ events, shouldClose }` |

多窗口手动画循环：每帧先 `pumpEvents()`，再对各窗口 `pollEvents` → 绘制 → `present()`；配合 `timers` 的 `setInterval`（见 `examples/multi_windows.js`）。阻塞式循环见 **`game`**（`game.run` / `game.isKeyDown`）。

依赖：`third_party/sdl2`、`third_party/nanovg`。无头：`QIANJS_NULL_UI=1`（录命令、不创建 GL）。
