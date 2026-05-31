#pragma once

#include "platform/draw_list.h"

#include <qjs/result.h>
#include <qjs/value.h>

#include <string>

namespace qianjs::canvas {

qjs::Result<platform::Color4> parse_color(const qjs::Value& v);
qjs::Result<platform::Color4> parse_color_string(std::string s);

} // namespace qianjs::canvas
