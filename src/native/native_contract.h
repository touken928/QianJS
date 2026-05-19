#pragma once

/**
 * Native module contract (all built-in plugins under src/native/).
 *
 * - Surface: one *Plugin class per module implementing qjs::IPlugin (install on JSEngine).
 * - Registration: CMake catalog -> qianjs_populate_default_plugins() (topological order).
 * - Threading: install() and JS callbacks on the runtime script thread; async I/O uses
 *   qianjs::event_loop::defer() (see fs, timers).
 * - Dependencies: declared in native_modules.cmake (DEPS between modules, REQUIRES libuv/ui_stack).
 * - Build: QIANJS_PROFILE (minimal|io|desktop) or per-module QIANJS_MODULE_* when profile is custom.
 * - Observability: QIANJS_BUILD_PROFILE and QIANJS_BUILD_MODULES in <qianjs_modules.h>.
 */
