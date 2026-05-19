# Fs 模块（精简异步 / 同步子模块）

文件系统分为两层：

- **`fs`**：返回 **Promise** 的异步 API（libuv `uv_fs_*`，完成回调经 `event_loop::defer` 回到 JS 线程）。
- **`fs.sync`**：同名函数的 **同步** 版本（阻塞当前线程；`stat` 使用 libuv 同步 `uv_fs_stat`，其余以 `std::filesystem` / 流为主）。

不设 `options` / `flag` / `mode` 等参数：语义固定，便于维护与使用。

## 导入

```javascript
import * as fs from 'fs';
import { sync as fss } from 'fs';
```

C++ 侧将 **`sync`** 作为 **`fs` 的子导出**（命名空间对象），因此使用 `import { sync } from 'fs'` 或 `fs.sync.readFile(...)`。顶层模块名只有 **`fs`**，没有单独的 `fs/sync` 模块 specifier。

## 二进制与文本

- **`readFile(path)`**：`Promise<string>`，按 **UTF-8** 解码（与原先带 `encoding: 'utf8'` 行为一致）。
- **`readFileBytes(path)`**：`Promise<ArrayBuffer>` 原始字节；需要视图时用 **`new Uint8Array(buf)`**（无 Node `Buffer`）。
- **`writeFile(path, data)`**：覆盖写入；**`data`** 可为字符串、`ArrayBuffer` 或 TypedArray。权限为运行时默认（类 Unix 上新建文件约 `0o644`）。

## 异步 API（`fs`）

| 函数 | 说明 |
|------|------|
| `readFile(path)` | 读整个文件为 UTF-8 字符串。 |
| `readFileBytes(path)` | 读整个文件为 `ArrayBuffer`。 |
| `writeFile(path, data)` | 截断写入。 |
| `mkdir(path)` | 创建单级目录（父目录须已存在）。 |
| `mkdirRecursive(path)` | 递归创建目录；异步分支在实现上等价于 `std::filesystem::create_directories`。 |
| `readdir(path)` | `Promise<string[]>`，仅文件名（不含 `.` / `..`）。 |
| `stat(path)` | 跟随符号链接；结果为普通对象（字段同 Node `fs.Stats` 的常见标量/布尔，无原型方法）。 |
| `unlink(path)` | 删除文件（目录会失败）。 |
| `rmdir(path)` | 删除**空**目录。 |

## 同步 API（`fs.sync`）

与上表同名、同参数；返回值直接为结果类型（`void` 操作为 `undefined`），失败时 **抛出**（QuickJS 异常）。

## 示例

```javascript
import { log } from 'console';
import * as fs from 'fs';

const text = await fs.readFile('README.md');
const raw = await fs.readFileBytes('README.md');
log('utf8 length ' + text.length);
log('bytes ' + raw.byteLength);

const enc = new TextEncoder().encode('hello');
await fs.writeFile('out.bin', enc);

await fs.mkdirRecursive('dist/nested');
const names = await fs.readdir('.');
const st = await fs.stat('README.md');
log('stat ' + st.isFile + ' ' + st.size + ' ' + st.mtimeMs);

// 同步
const t2 = fs.sync.readFile('README.md');
fs.sync.writeFile('out2.txt', 'hi');
```

进程退出前仍会排空挂起的 `fs` 异步操作（`Application::run_script` → `RuntimeInstance::run_until_idle()`）。

## 与 Node 的差异（简要）

- 无 `fs.promises` 命名空间；异步即顶层 `fs.*`。
- 无 `Buffer`；二进制为 `ArrayBuffer` + 可选 `Uint8Array`。
- 已移除：`appendFile`、`lstat`、`rename`、`copyFile` 及各类可选参数；需要追加/重命名/拷贝时请用读写组合或其它方式自行实现。
