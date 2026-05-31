#pragma once

#include <qjs/value.h>

namespace qianjs {

struct FrameLoopOptions {
    int width = 640;
    int height = 480;
    const char* title = "QianJS";
    int64_t max_frames = -1;
    double target_fps = 0;
    double fixed_dt = 0;
};

/** Holds JS app lifecycle callbacks (init / update / render / shutdown). */
class AppHost {
public:
    AppHost() = default;
    ~AppHost();

    AppHost(const AppHost&) = delete;
    AppHost& operator=(const AppHost&) = delete;

    bool load_from_object(qjs::Value app_obj);
    void release();

    bool has_hooks() const;

    const qjs::Value& init_fn() const { return init_; }
    const qjs::Value& update_fn() const { return update_; }
    const qjs::Value& render_fn() const { return render_; }
    const qjs::Value& shutdown_fn() const { return shutdown_; }

private:
    static qjs::Value take_fn(const qjs::Value& obj, const char* key);

    qjs::Value init_{};
    qjs::Value update_{};
    qjs::Value render_{};
    qjs::Value shutdown_{};
};

} // namespace qianjs
