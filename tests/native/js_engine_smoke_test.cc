#include <gtest/gtest.h>

#include <qjs/engine.h>

TEST(JsEngineSmoke, ConstructAndEvalModule) {
    qjs::Engine engine;
    EXPECT_TRUE(engine.evalModule("m.js", "export {};\n").success);
}
