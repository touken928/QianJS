#pragma once

#include <quickjs.h>

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

  bool load_from_object(JSContext* c, JSValue app_obj);
  void release(JSContext* c);

  bool has_hooks() const;

  JSValue init_fn() const { return init_; }
  JSValue update_fn() const { return update_; }
  JSValue render_fn() const { return render_; }
  JSValue shutdown_fn() const { return shutdown_; }

private:
  static JSValue dup_fn(JSContext* c, JSValue obj, const char* key);

  JSValue init_ = JS_UNDEFINED;
  JSValue update_ = JS_UNDEFINED;
  JSValue render_ = JS_UNDEFINED;
  JSValue shutdown_ = JS_UNDEFINED;
};

} // namespace qianjs
