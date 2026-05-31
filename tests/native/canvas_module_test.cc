#include <qianjs_modules.h>

#if !QIANJS_MODULE_CANVAS
#error "canvas_module_test requires QIANJS_MODULE_CANVAS"
#endif

#include "native/js_test_harness.h"

#include <gtest/gtest.h>

#include <cstdlib>

TEST(NativeCanvasModule, CreateCanvasAndDraw) {
    (void)setenv("QIANJS_NULL_UI", "1", 1);

    const char* src = R"(
import * as canvas from 'canvas';
const c = canvas.createCanvas(64, 48, { title: 'test' });
const ctx = c.getContext('2d');
if (!ctx) throw new Error('no 2d');
ctx.setFillStyle('rgb(10, 20, 30)');
ctx.fillRect(0, 0, 64, 48);
)";

    qianjs::test::TestRuntime rt({"prog"}, {});
    ASSERT_TRUE(qianjs::test::run_module(rt.engine(), "canvas_test.js", src));
    rt.drain();
}
