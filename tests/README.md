# 测试目录（对齐 `src/`）

| `tests/` 路径 | 对应 `src/` | 说明 |
|---------------|---------------|------|
| `runtime/` | `src/runtime/` | 无 JS 运行时的单测（如 `embed`、宿主上下文） |
| `cli/` | `src/cli/` | `cli_test.cc`：`qianjs_cli_run()` 路由与退出码（需 **`QIANJS_BUILD_CLI=ON`** + 链接 **`qianjs_impl`**） |
| `native/` | `src/native/` | 每插件一 `*_module_test.cc`（`#if QIANJS_MODULE_*`），覆盖各模块当前导出的 API（含 `fs` / `fs.sync`）；`js_engine_smoke_test.cc`、`js_test_harness.h`。不含 `ui`；需 **`QIANJS_BUILD_CLI=ON`** |

在 **`tests/CMakeLists.txt`** 的 `QIANJS_TEST_SOURCES` 中登记新文件。根目录开启 **`QIANJS_BUILD_TESTS`** 时构建 **`qianjs_tests`** 并注册 **`ctest`**。

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j8
ctest --test-dir build --output-on-failure
```
