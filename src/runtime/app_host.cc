#include "runtime/app_host.h"

namespace qianjs {

AppHost::~AppHost() {
    release();
}

qjs::Value AppHost::take_fn(const qjs::Value& obj, const char* key) {
    auto prop = obj.getProperty(key);
    if (!prop.success) {
        return {};
    }
    if (!prop.value.isFunction()) {
        return {};
    }
    return std::move(prop.value);
}

bool AppHost::load_from_object(qjs::Value app_obj) {
    release();
    if (!app_obj.isObject()) {
        return false;
    }

    init_ = take_fn(app_obj, "init");
    update_ = take_fn(app_obj, "update");
    render_ = take_fn(app_obj, "render");
    shutdown_ = take_fn(app_obj, "shutdown");

    if (!update_.isFunction() || !render_.isFunction()) {
        release();
        return false;
    }
    return true;
}

void AppHost::release() {
    init_ = {};
    update_ = {};
    render_ = {};
    shutdown_ = {};
}

bool AppHost::has_hooks() const {
    return update_.isFunction() && render_.isFunction();
}

} // namespace qianjs
