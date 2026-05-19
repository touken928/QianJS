# timers 模块（基础定时器）

提供 `setTimeout` / `setInterval` 以及对应清理函数。`qianjs run` 在脚本结束后会调用 **`RuntimeInstance::run_until_idle()`**（排空 libuv、defer 与微任务）。

## 导入

```javascript
import { setTimeout, setInterval, clearTimeout, clearInterval } from 'timers';
```

## API

### `setTimeout(callback, delayMs)`

- `callback`：无参数函数。
- `delayMs`：毫秒，负数按 `0` 处理。
- 返回：`number`（timer id），供 `clearTimeout` 使用。
- 实现：`uv_timer_t` 在共享 libuv 循环上触发，经 `event_loop::defer` 在 **JS 主线程** 执行回调。

### `setInterval(callback, delayMs)`

- `callback`：无参数函数。
- `delayMs`：毫秒，负数按 `0`；周期为 `0` 时在实现中会抬到 **1ms**，避免忙等。
- 返回：`number`（timer id）。

### `clearTimeout(id)` / `clearInterval(id)`

- 取消对应 timer；对无效或已结束的 id **幂等**，可重复调用。

## 插件初始化

加载本模块时会调用 `event_loop::ensure_started()`；与 `fs`（若启用）共用同一 libuv 循环。

## 示例

```javascript
import { setTimeout, setInterval, clearInterval } from 'timers';
import { log } from 'console';

setTimeout(() => {
  log('once');
}, 50);

let count = 0;
const id = setInterval(() => {
  count++;
  log('tick', count);
  if (count >= 3) clearInterval(id);
}, 20);
```

## 说明

- 与 Node 不同：不提供全局 `setTimeout`，须从 `'timers'` 导入。
- 长时间运行的 `setInterval` 应在脚本结束前 `clearInterval`；宿主在 `shutdown` 时会取消未触发的 timer。
