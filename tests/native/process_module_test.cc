#include <qianjs_modules.h>

#if QIANJS_MODULE_PROCESS

#include <gtest/gtest.h>

#include "native/js_test_harness.h"

#include <string>
#include <utility>
#include <vector>

TEST(NativeProcessModule, ArgvPidPlatformCwd) {
    qianjs::test::TestRuntime rt(
        {std::string("prog"), std::string("one"), std::string("two")},
        {{"QIANJS_NATIVE_TEST_K", "native_val"}});
    qjs::JSEngine& engine = rt.engine();

    const std::string js = R"(
import { argv, pid, platform, cwd } from 'process';
globalThis.__argv = JSON.stringify(argv());
globalThis.__pid = String(pid());
globalThis.__platform = platform();
globalThis.__cwd = cwd();
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_process_argv.mjs", js));
    rt.drain();

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
}

TEST(NativeProcessModule, EnvLookupAndFullObject) {
    qianjs::test::TestRuntime rt(
        {"prog"},
        {{"QIANJS_PTEST_A", "v1"}, {"QIANJS_PTEST_B", "v2"}, {"QIANJS_NATIVE_TEST_K", "native_val"}});
    qjs::JSEngine& engine = rt.engine();

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
    rt.drain();

    JSContext* c = engine.ctx();
    EXPECT_EQ(qianjs::test::global_string(c, "__env_a"), "v1");
    EXPECT_EQ(qianjs::test::global_string(c, "__env_b"), "v2");
    EXPECT_EQ(qianjs::test::global_string(c, "__env_missing"), "yes");
    EXPECT_EQ(qianjs::test::global_string(c, "__env_keys"), R"(["QIANJS_PTEST_A","QIANJS_PTEST_B"])");
}

TEST(NativeProcessModule, SetGetExitCodeAndExitCodeAlias) {
    qianjs::test::TestRuntime rt({"prog"}, {});
    qjs::JSEngine& engine = rt.engine();

    const std::string js = R"(
import { getExitCode, exitCode, setExitCode } from 'process';
setExitCode(7);
globalThis.__after_set = String(getExitCode());
globalThis.__alias = String(exitCode());
setExitCode(-3);
globalThis.__negative = String(getExitCode());
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_process_exit.mjs", js));
    rt.drain();

    JSContext* c = engine.ctx();
    EXPECT_EQ(rt.instance.host().exit_code, -3);
    EXPECT_EQ(qianjs::test::global_string(c, "__after_set"), "7");
    EXPECT_EQ(qianjs::test::global_string(c, "__alias"), "7");
    EXPECT_EQ(qianjs::test::global_string(c, "__negative"), "-3");
}

#endif // QIANJS_MODULE_PROCESS
