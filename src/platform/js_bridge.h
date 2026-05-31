#pragma once

#include <qjs/engine.h>
#include <qjs/value.h>

#include <SDL.h>

#include <vector>

namespace qianjs::platform {

qjs::Value sdl_event_to_value(qjs::Engine& engine, const SDL_Event& e);
qjs::Value events_to_value(qjs::Engine& engine, const std::vector<SDL_Event>& events);
qjs::Value mouse_state_value(qjs::Engine& engine, bool null_mode);
qjs::Value mod_state_value(qjs::Engine& engine, bool null_mode);
qjs::Value is_key_down_value(qjs::Engine& engine, bool null_mode, const qjs::Value& key_arg);

} // namespace qianjs::platform
