#include "runtime/app_host.h"

namespace qianjs {

AppHost::~AppHost() {
    init_ = JS_UNDEFINED;
    update_ = JS_UNDEFINED;
    render_ = JS_UNDEFINED;
    shutdown_ = JS_UNDEFINED;
}

JSValue AppHost::dup_fn(JSContext* c, JSValue obj, const char* key) {
    JSValue fn = JS_GetPropertyStr(c, obj, key);
    if (JS_IsException(fn)) {
        return JS_EXCEPTION;
    }
    if (!JS_IsFunction(c, fn)) {
        JS_FreeValue(c, fn);
        return JS_UNDEFINED;
    }
    return fn;
}

bool AppHost::load_from_object(JSContext* c, JSValue app_obj) {
    release(c);
    if (!JS_IsObject(app_obj)) {
        return false;
    }

    init_ = dup_fn(c, app_obj, "init");
    if (JS_IsException(init_)) {
        release(c);
        return false;
    }
    update_ = dup_fn(c, app_obj, "update");
    if (JS_IsException(update_)) {
        release(c);
        return false;
    }
    render_ = dup_fn(c, app_obj, "render");
    if (JS_IsException(render_)) {
        release(c);
        return false;
    }
    shutdown_ = dup_fn(c, app_obj, "shutdown");
    if (JS_IsException(shutdown_)) {
        release(c);
        return false;
    }

    if (JS_IsUndefined(update_) || JS_IsUndefined(render_)) {
        release(c);
        return false;
    }
    return true;
}

void AppHost::release(JSContext* c) {
    if (c) {
        if (!JS_IsUndefined(init_)) {
            JS_FreeValue(c, init_);
        }
        if (!JS_IsUndefined(update_)) {
            JS_FreeValue(c, update_);
        }
        if (!JS_IsUndefined(render_)) {
            JS_FreeValue(c, render_);
        }
        if (!JS_IsUndefined(shutdown_)) {
            JS_FreeValue(c, shutdown_);
        }
    }
    init_ = JS_UNDEFINED;
    update_ = JS_UNDEFINED;
    render_ = JS_UNDEFINED;
    shutdown_ = JS_UNDEFINED;
}

bool AppHost::has_hooks() const {
    return !JS_IsUndefined(update_) && !JS_IsUndefined(render_);
}

} // namespace qianjs
