#include <qianjs_modules.h>

#if QIANJS_MODULE_PROCESS

#include <gtest/gtest.h>

#include "native/js_test_harness.h"

#include <string>
#include <utility>
#include <vector>

TEST(NativeProcessModule, ArgvPidPlatformCwd) {
    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(
        engine, runtime,
        {std::string("prog"), std::string("one"), std::string("two")},
        {{"QIANJS_NATIVE_TEST_K", "native_val"}});

    const std::string js = R"(
import { argv, pid, platform, cwd } from 'process';
globalThis.__argv = JSON.stringify(argv());
globalThis.__pid = String(pid());
globalThis.__platform = platform();
globalThis.__cwd = cwd();
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_process_argv.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    EXPECT_EQ(qianjs::test::global_string(c, "__argv"), R"(["prog","one","two"])");
    EXPECT_NE(qianjs::test::global_string(c, "__pid"), "");
    const std::string plat = qianjs::test::global_string(c, "__platform");
#if defined(_WIN32)
    EXPECT_EQ(plat, "win32");
#elif defined(__APPLE__)
    EXPECT_EQ(plat, "darwin");
#else
    EXPECT_EQ(plat, "linux");
#endif
    EXPECT_NE(qianjs::test::global_string(c, "__cwd"), "");

    engine.cleanup();
}

TEST(NativeProcessModule, EnvLookupAndFullObject) {
    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"},
        {{"QIANJS_PTEST_A", "v1"}, {"QIANJS_PTEST_B", "v2"}, {"QIANJS_NATIVE_TEST_K", "native_val"}});

    const std::string js = R"(
import { env } from 'process';
const o = env();
globalThis.__env_a = env('QIANJS_PTEST_A');
globalThis.__env_b = env('QIANJS_PTEST_B');
globalThis.__env_missing = env('__no_such_key___') === undefined ? 'yes' : 'no';
globalThis.__env_keys = JSON.stringify(
  Object.keys(o).filter((k) => k.startsWith('QIANJS_PTEST')).sort()
);
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_process_env.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    EXPECT_EQ(qianjs::test::global_string(c, "__env_a"), "v1");
    EXPECT_EQ(qianjs::test::global_string(c, "__env_b"), "v2");
    EXPECT_EQ(qianjs::test::global_string(c, "__env_missing"), "yes");
    EXPECT_EQ(qianjs::test::global_string(c, "__env_keys"), R"(["QIANJS_PTEST_A","QIANJS_PTEST_B"])");

    engine.cleanup();
}

TEST(NativeProcessModule, SetGetExitCodeAndExitCodeAlias) {
    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    const std::string js = R"(
import { getExitCode, exitCode, setExitCode } from 'process';
setExitCode(7);
globalThis.__after_set = String(getExitCode());
globalThis.__alias = String(exitCode());
setExitCode(-3);
globalThis.__negative = String(getExitCode());
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_process_exit.mjs", js));
    qianjs::drainAsyncWork(engine);

    JSContext* c = engine.ctx();
    EXPECT_EQ(runtime.exit_code, -3);
    EXPECT_EQ(qianjs::test::global_string(c, "__after_set"), "7");
    EXPECT_EQ(qianjs::test::global_string(c, "__alias"), "7");
    EXPECT_EQ(qianjs::test::global_string(c, "__negative"), "-3");

    engine.cleanup();
}

#endif // QIANJS_MODULE_PROCESS
