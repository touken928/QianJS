#pragma once

#include <qjs/engine.h>
#include <qianjs_default_plugins.g.h>

inline void installDefaultPlugins(qjs::Engine& engine) {
    qianjs_install_default_plugins(engine);
}
