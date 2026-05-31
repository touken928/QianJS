#pragma once

#include <qjs/plugin.h>
#include <qianjs_default_plugins.g.h>

inline qjs::PluginRegistry defaultPlugins() {
    qjs::PluginRegistry r;
    qianjs_populate_default_plugins(r);
    return r;
}
