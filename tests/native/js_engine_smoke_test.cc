#include <gtest/gtest.h>

#include <js_engine.h>

/** Minimal JS engine lifecycle check (no native plugins). */
TEST(NativeJsEngineSmoke, InitializesAndCleansUp) {
    qjs::JSEngine engine;
    engine.initialize();
    engine.cleanup();
}
