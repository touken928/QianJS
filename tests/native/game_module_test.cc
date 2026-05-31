#include <qianjs_modules.h>

#if !QIANJS_MODULE_GAME
#error "game_module_test requires QIANJS_MODULE_GAME"
#endif

#include "native/js_test_harness.h"

#include <gtest/gtest.h>

#include <cstdlib>

TEST(NativeGameModule, RunHeadless) {
    (void)setenv("QIANJS_NULL_UI", "1", 1);

    const char* src = R"(
import * as canvas from 'canvas';
import * as game from 'game';

const c = canvas.createCanvas(64, 48, { title: 'test' });
const ctx = c.getContext('2d');
let frames = 0;

game.run(c, {
    update() { frames++; },
    render() {
        ctx.setFillStyle('rgb(1, 2, 3)');
        ctx.fillRect(0, 0, 64, 48);
    },
}, { maxFrames: 2, fps: 0 });

if (frames < 1) throw new Error('expected at least one update');
)";

    qianjs::test::TestRuntime rt({"prog"}, {});
    ASSERT_TRUE(qianjs::test::run_module(rt.engine(), "game_test.js", src));
    rt.drain();
}
