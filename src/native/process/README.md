# process 模块（最小运行时信息）

提供运行参数、环境变量快照、当前进程元数据与退出码控制。与 `console`、`timers` 一起构成推荐保留的**核心三模块**。

## 导入

```javascript
import * as process from 'process';
```

## API

### `argv()`

- 返回：`string[]`，脚本参数数组。
- `qianjs run app.js a b` 时，返回 `["app.js", "a", "b"]`（由 **`Application::run_script`** 写入 **`RuntimeInstance::host().argv`**）。

### `env()`

- 返回：`Record<string, string>`，**启动时**捕获的环境变量快照（只读视图；重开进程前对系统 `environ` 的修改不会反映到已创建的快照里）。

### `env(key)`

- `key`：`string`。
- 返回：对应环境变量字符串；不存在时返回 `undefined`。

### `pid()`

- 返回：当前进程 id（`number`），与系统 `getpid` / Windows 等价 API 一致。

### `platform()`

- 返回：小写平台 id 字符串，与常见 Node 习惯对齐之一致子集：
  - `darwin`（macOS）
  - `linux`
  - `win32`（含 MinGW 等）

### `cwd()`

- 返回：当前工作目录路径（`string`）。若无法取得（极少见），可能返回空字符串。

### `getExitCode()` / `exitCode()`

- 返回：当前为进程结束准备的退出码（`number`，默认 `0`）。`exitCode` 为只读别名，与 `getExitCode` 相同。

### `setExitCode(code)`

- `code`：`number`。
- 设置宿主在脚本结束后返回给 shell 的退出码（不会立即终止进程）。

## 示例

```javascript
import * as process from 'process';
import { log } from 'console';

log('pid', process.pid(), 'cwd', process.cwd(), 'plat', process.platform());

const args = process.argv();
log('args:', args.length);

const home = process.env('HOME') ?? process.env('USERPROFILE') ?? '';
log('home:', home);

if (args.includes('--fail')) {
  process.setExitCode(2);
}
```

## 说明

- 非 Node 的 `process.argv` / `process.env` **属性**形态；此处为函数式导出，便于当前插件模型下的稳定绑定。
- 不提供 `process.exit()`：终止时机由宿主在脚本跑完后统一收尾；请用 `setExitCode` 表达期望退出码。
