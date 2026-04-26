#include <qianjs_modules.h>

#if QIANJS_MODULE_CONSOLE

#include <gtest/gtest.h>

#include "native/js_test_harness.h"

#include <sstream>
#include <string>

TEST(NativeConsoleModule, LogInfoDebugGoToStdout) {
    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    std::stringstream cap;
    std::streambuf* prev = std::cout.rdbuf(cap.rdbuf());

    const std::string js = R"(
import { log, info, debug } from 'console';
log('L', 1, true);
info('I');
debug('D');
)";

    const bool ran = qianjs::test::run_module(engine, "native_console_out.mjs", js);
    std::cout.rdbuf(prev);
    ASSERT_TRUE(ran);
    qianjs::drainAsyncWork(engine);

    const std::string out = cap.str();
    EXPECT_NE(out.find("L 1 true"), std::string::npos);
    EXPECT_NE(out.find("I"), std::string::npos);
    EXPECT_NE(out.find("D"), std::string::npos);

    engine.cleanup();
}

TEST(NativeConsoleModule, LogWithNoArgsWritesNewline) {
    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    std::stringstream cap;
    std::streambuf* prev = std::cout.rdbuf(cap.rdbuf());

    const std::string js = R"(
import { log } from 'console';
log();
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_console_empty_log.mjs", js));
    std::cout.rdbuf(prev);
    qianjs::drainAsyncWork(engine);

    EXPECT_EQ(cap.str(), "\n");

    engine.cleanup();
}

TEST(NativeConsoleModule, WarnErrorGoToStderr) {
    qjs::JSEngine engine;
    engine.initialize();
    qianjs::RuntimeContext runtime;
    qianjs::test::install_plugins(engine, runtime, {"prog"}, {});

    std::stringstream cap;
    std::streambuf* prev = std::cerr.rdbuf(cap.rdbuf());

    const std::string js = R"(
import { warn, error } from 'console';
warn('W', 2);
error('E');
)";

    const bool ran = qianjs::test::run_module(engine, "native_console_err.mjs", js);
    std::cerr.rdbuf(prev);
    ASSERT_TRUE(ran);
    qianjs::drainAsyncWork(engine);

    const std::string err = cap.str();
    EXPECT_NE(err.find("W 2"), std::string::npos);
    EXPECT_NE(err.find("E"), std::string::npos);

    engine.cleanup();
}

#endif // QIANJS_MODULE_CONSOLE
