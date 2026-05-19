#include <qianjs_modules.h>

#if QIANJS_MODULE_CONSOLE

#include <gtest/gtest.h>

#include "native/js_test_harness.h"

#include <sstream>
#include <string>

TEST(NativeConsoleModule, LogInfoDebugGoToStdout) {
    qianjs::test::TestRuntime rt({"prog"}, {});
    qjs::JSEngine& engine = rt.engine();

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
    rt.drain();

    const std::string out = cap.str();
    EXPECT_NE(out.find("L 1 true"), std::string::npos);
    EXPECT_NE(out.find("I"), std::string::npos);
    EXPECT_NE(out.find("D"), std::string::npos);

}

TEST(NativeConsoleModule, LogWithNoArgsWritesNewline) {
    qianjs::test::TestRuntime rt({"prog"}, {});
    qjs::JSEngine& engine = rt.engine();

    std::stringstream cap;
    std::streambuf* prev = std::cout.rdbuf(cap.rdbuf());

    const std::string js = R"(
import { log } from 'console';
log();
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_console_empty_log.mjs", js));
    std::cout.rdbuf(prev);
    rt.drain();

    EXPECT_EQ(cap.str(), "\n");

}

TEST(NativeConsoleModule, WarnErrorGoToStderr) {
    qianjs::test::TestRuntime rt({"prog"}, {});
    qjs::JSEngine& engine = rt.engine();

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
    rt.drain();

    const std::string err = cap.str();
    EXPECT_NE(err.find("W 2"), std::string::npos);
    EXPECT_NE(err.find("E"), std::string::npos);

}

#endif // QIANJS_MODULE_CONSOLE
