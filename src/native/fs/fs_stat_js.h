#pragma once

#include <qjs/engine.h>
#include <qjs/value.h>

#include <uv.h>

/** Plain object shaped like Node `fs.Stats` (fields only, no methods). */
qjs::Value fs_stat_to_value(qjs::Engine& engine, const uv_stat_t& st);
