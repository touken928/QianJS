#pragma once

#include "platform/canvas_registry.h"
#include "runtime/instance.h"

#include <qjs/engine.h>
#include <qjs/value.h>

namespace qianjs::canvas {

inline platform::CanvasRegistry* registry_from_instance() {
    qianjs::RuntimeInstance* inst = qianjs::RuntimeInstance::current();
    return inst ? &inst->canvases() : nullptr;
}

inline uint64_t canvas_id_from_value(const qjs::Value& v) {
    auto id = v.getProperty("_canvasId");
    if (!id.success || id.value.isUndefined()) {
        return 0;
    }
    auto n = id.value.toInt64();
    return n.success ? static_cast<uint64_t>(n.value) : 0;
}

inline platform::PlatformCanvas* canvas_by_id(uint64_t id) {
    platform::CanvasRegistry* reg = registry_from_instance();
    return reg ? reg->get(id) : nullptr;
}

inline platform::PlatformCanvas* require_canvas(qjs::Engine& engine, const qjs::Value& canvas_obj) {
    platform::CanvasRegistry* reg = registry_from_instance();
    if (!reg) {
        (void)engine.throwTypeError("canvas: no active runtime instance");
        return nullptr;
    }
    const uint64_t id = canvas_id_from_value(canvas_obj);
    platform::PlatformCanvas* c = reg->get(id);
    if (!c || !c->inited()) {
        (void)engine.throwTypeError("canvas: invalid or destroyed canvas");
        return nullptr;
    }
    return c;
}

} // namespace qianjs::canvas
