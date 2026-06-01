#pragma once

/**
 * Native module contract (all built-in plugins under src/native/).
 *
 * - Surface: one *Plugin class per module implementing qjs::IPlugin (install on Context).
 * - Registration: CMake catalog -> qianjs_install_default_plugins(engine) (topological order).
 * - Threading: install() and JS callbacks on the runtime script thread; async I/O uses
 *   qianjs::event_loop::defer() (see fs, timers).
 * - Dependencies: declared in native_modules.cmake (DEPS between modules, REQUIRES libuv/ui_stack).
 * - Build: QIANJS_PROFILE (minimal|io|desktop) or per-module QIANJS_MODULE_* when profile is custom.
 * - Observability: QIANJS_BUILD_PROFILE and QIANJS_BUILD_MODULES in <qianjs_modules.h>.
 *
 * qjs boundary (see AGENTS.md):
 * - Native plugins use only opaque qjs APIs (Module::func, Value, Engine::call, ObjectBuilder).
 * - Do not include quickjs.h or use JSContext/JSValue in src/native/ or src/runtime/ (platform
 *   SDL→JS helpers use Engine + js_bridge.cc).
 * - qjs does not implement libuv/SDL, argv/embed, or shutdown; QianJS owns those.
 */
