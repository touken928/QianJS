#include <qianjs_modules.h>

#if QIANJS_MODULE_TIMERS

#include <gtest/gtest.h>

#include "native/js_test_harness.h"

#include <string>

TEST(NativeTimersModule, SetTimeoutReturnsClearableId) {
    qianjs::test::TestRuntime rt({"prog"}, {});
    qjs::Engine& engine = rt.engine();

    const std::string js = R"(
import { setTimeout, clearTimeout } from 'timers';
const id = setTimeout(() => { globalThis.__tid_hit = 1; }, 60000);
globalThis.__tid_type = typeof id;
clearTimeout(id);
globalThis.__tid_cleared = 1;
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_timers_id.mjs", js));
    rt.drain();

    
    const std::string ty = qianjs::test::global_string(engine, "__tid_type");
    EXPECT_TRUE(ty == "number" || ty == "bigint");
    bool ok = false;
    EXPECT_EQ(qianjs::test::global_int(engine, "__tid_cleared", &ok), 1);
    EXPECT_TRUE(ok);
    EXPECT_EQ(qianjs::test::global_string(engine, "__tid_hit"), "");

}

TEST(NativeTimersModule, SetTimeoutFires) {
    qianjs::test::TestRuntime rt({"prog"}, {});
    qjs::Engine& engine = rt.engine();

    const std::string js = R"(
import { setTimeout } from 'timers';
setTimeout(() => { globalThis.__timer_ok = 1; }, 5);
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_timers_timeout.mjs", js));
    rt.drain();

    
    bool ok = false;
    const int n = qianjs::test::global_int(engine, "__timer_ok", &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(n, 1);

}

TEST(NativeTimersModule, SetTimeoutZeroDelay) {
    qianjs::test::TestRuntime rt({"prog"}, {});
    qjs::Engine& engine = rt.engine();

    const std::string js = R"(
import { setTimeout } from 'timers';
setTimeout(() => { globalThis.__z = 1; }, 0);
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_timers_zero.mjs", js));
    rt.drain();

    
    bool ok = false;
    EXPECT_EQ(qianjs::test::global_int(engine, "__z", &ok), 1);
    EXPECT_TRUE(ok);

}

TEST(NativeTimersModule, SetIntervalThenClearInterval) {
    qianjs::test::TestRuntime rt({"prog"}, {});
    qjs::Engine& engine = rt.engine();

    const std::string js = R"(
import { setInterval, clearInterval } from 'timers';
let n = 0;
const id = setInterval(() => {
  n++;
  globalThis.__ival = n;
  if (n >= 3) clearInterval(id);
}, 12);
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_timers_interval.mjs", js));
    rt.drain();

    
    bool ok = false;
    const int n = qianjs::test::global_int(engine, "__ival", &ok);
    EXPECT_TRUE(ok);
    EXPECT_GE(n, 3);

}

TEST(NativeTimersModule, ClearTimeoutBeforeFire) {
    qianjs::test::TestRuntime rt({"prog"}, {});
    qjs::Engine& engine = rt.engine();

    const std::string js = R"(
import { setTimeout, clearTimeout } from 'timers';
const id = setTimeout(() => { globalThis.__long = 1; }, 60000);
clearTimeout(id);
globalThis.__cleared_ok = 1;
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_timers_clear.mjs", js));
    rt.drain();

    
    bool ok = false;
    EXPECT_EQ(qianjs::test::global_int(engine, "__cleared_ok", &ok), 1);
    EXPECT_TRUE(ok);
    EXPECT_EQ(qianjs::test::global_string(engine, "__long"), "");

}

TEST(NativeTimersModule, ClearTimeoutUnknownIdNoOp) {
    qianjs::test::TestRuntime rt({"prog"}, {});
    qjs::Engine& engine = rt.engine();

    const std::string js = R"(
import { clearTimeout } from 'timers';
clearTimeout(999999999);
globalThis.__noop_clear = 1;
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_timers_clear_unknown.mjs", js));
    rt.drain();

    
    bool ok = false;
    EXPECT_EQ(qianjs::test::global_int(engine, "__noop_clear", &ok), 1);
    EXPECT_TRUE(ok);

}

TEST(NativeTimersModule, ClearIntervalAliasClearsTimer) {
    qianjs::test::TestRuntime rt({"prog"}, {});
    qjs::Engine& engine = rt.engine();

    const std::string js = R"(
import { setTimeout, clearInterval } from 'timers';
const id = setTimeout(() => { globalThis.__ci = 1; }, 60000);
clearInterval(id);
globalThis.__ci_cleared = 1;
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_timers_clear_interval_alias.mjs", js));
    rt.drain();

    
    bool ok = false;
    EXPECT_EQ(qianjs::test::global_int(engine, "__ci_cleared", &ok), 1);
    EXPECT_TRUE(ok);

}

TEST(NativeTimersModule, SetTimeoutNonFunctionThrowsTypeError) {
    qianjs::test::TestRuntime rt({"prog"}, {});
    qjs::Engine& engine = rt.engine();

    const std::string js = R"(
import { setTimeout } from 'timers';
try {
  setTimeout(123, 0);
  globalThis.__bad_to = 'no_throw';
} catch (e) {
  globalThis.__bad_to = String(e);
}
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_timers_bad_cb.mjs", js));
    rt.drain();

    
    const std::string msg = qianjs::test::global_string(engine, "__bad_to");
    EXPECT_NE(msg.find("callback must be function"), std::string::npos);

}

TEST(NativeTimersModule, SetIntervalNonFunctionThrowsTypeError) {
    qianjs::test::TestRuntime rt({"prog"}, {});
    qjs::Engine& engine = rt.engine();

    const std::string js = R"(
import { setInterval } from 'timers';
try {
  setInterval({}, 10);
  globalThis.__bad_iv = 'no_throw';
} catch (e) {
  globalThis.__bad_iv = String(e);
}
)";

    ASSERT_TRUE(qianjs::test::run_module(engine, "native_timers_bad_interval.mjs", js));
    rt.drain();

    
    const std::string msg = qianjs::test::global_string(engine, "__bad_iv");
    EXPECT_NE(msg.find("callback must be function"), std::string::npos);

}

#endif // QIANJS_MODULE_TIMERS
