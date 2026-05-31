#include "native/canvas/canvas_color.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>

namespace qianjs::canvas {

namespace {

bool parse_byte(const std::string& s, size_t& i, int& out) {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
        ++i;
    }
    if (i >= s.size()) {
        return false;
    }
    char* end = nullptr;
    const long v = std::strtol(s.c_str() + i, &end, 10);
    if (end == s.c_str() + i) {
        return false;
    }
    i = static_cast<size_t>(end - s.c_str());
    out = static_cast<int>(v);
    return true;
}

float clamp01(float x) {
    return std::clamp(x, 0.0f, 1.0f);
}

platform::Color4 rgba255(int r, int g, int b, int a = 255) {
    return {clamp01(r / 255.0f), clamp01(g / 255.0f), clamp01(b / 255.0f), clamp01(a / 255.0f)};
}

} // namespace

qjs::Result<platform::Color4> parse_color_string(std::string s) {
    auto trim = [&]() {
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front()))) {
            s.erase(s.begin());
        }
        while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back()))) {
            s.pop_back();
        }
    };
    trim();
    if (s.empty()) {
        return qjs::Result<platform::Color4>::fail(qjs::ErrorInfo{"invalid color", {}, {}});
    }

    if (s == "black") {
        return qjs::Result<platform::Color4>::ok(rgba255(0, 0, 0));
    }
    if (s == "white") {
        return qjs::Result<platform::Color4>::ok(rgba255(255, 255, 255));
    }
    if (s == "red") {
        return qjs::Result<platform::Color4>::ok(rgba255(255, 0, 0));
    }
    if (s == "green") {
        return qjs::Result<platform::Color4>::ok(rgba255(0, 128, 0));
    }
    if (s == "blue") {
        return qjs::Result<platform::Color4>::ok(rgba255(0, 0, 255));
    }

    if (s[0] == '#') {
        if (s.size() == 4) {
            const int r = std::stoi(s.substr(1, 1), nullptr, 16) * 17;
            const int g = std::stoi(s.substr(2, 1), nullptr, 16) * 17;
            const int b = std::stoi(s.substr(3, 1), nullptr, 16) * 17;
            return qjs::Result<platform::Color4>::ok(rgba255(r, g, b));
        }
        if (s.size() == 7) {
            const int r = std::stoi(s.substr(1, 2), nullptr, 16);
            const int g = std::stoi(s.substr(3, 2), nullptr, 16);
            const int b = std::stoi(s.substr(5, 2), nullptr, 16);
            return qjs::Result<platform::Color4>::ok(rgba255(r, g, b));
        }
        if (s.size() == 9) {
            const int r = std::stoi(s.substr(1, 2), nullptr, 16);
            const int g = std::stoi(s.substr(3, 2), nullptr, 16);
            const int b = std::stoi(s.substr(5, 2), nullptr, 16);
            const int a = std::stoi(s.substr(7, 2), nullptr, 16);
            return qjs::Result<platform::Color4>::ok(rgba255(r, g, b, a));
        }
    }

    if (s.rfind("rgba(", 0) == 0 && s.back() == ')') {
        size_t i = 5;
        int r = 0;
        int g = 0;
        int b = 0;
        float a = 1.0f;
        if (!parse_byte(s, i, r) || s[i] != ',') {
            return qjs::Result<platform::Color4>::fail(qjs::ErrorInfo{"invalid rgba()", {}, {}});
        }
        ++i;
        if (!parse_byte(s, i, g) || s[i] != ',') {
            return qjs::Result<platform::Color4>::fail(qjs::ErrorInfo{"invalid rgba()", {}, {}});
        }
        ++i;
        if (!parse_byte(s, i, b) || s[i] != ',') {
            return qjs::Result<platform::Color4>::fail(qjs::ErrorInfo{"invalid rgba()", {}, {}});
        }
        ++i;
        char* end = nullptr;
        a = std::strtof(s.c_str() + i, &end);
        if (end == s.c_str() + i) {
            return qjs::Result<platform::Color4>::fail(qjs::ErrorInfo{"invalid rgba()", {}, {}});
        }
        if (a > 1.0f) {
            a /= 255.0f;
        }
        return qjs::Result<platform::Color4>::ok(platform::Color4{clamp01(r / 255.0f), clamp01(g / 255.0f), clamp01(b / 255.0f), clamp01(a)});
    }

    if (s.rfind("rgb(", 0) == 0 && s.back() == ')') {
        size_t i = 4;
        int r = 0;
        int g = 0;
        int b = 0;
        if (!parse_byte(s, i, r) || s[i] != ',') {
            return qjs::Result<platform::Color4>::fail(qjs::ErrorInfo{"invalid rgb()", {}, {}});
        }
        ++i;
        if (!parse_byte(s, i, g) || s[i] != ',') {
            return qjs::Result<platform::Color4>::fail(qjs::ErrorInfo{"invalid rgb()", {}, {}});
        }
        ++i;
        if (!parse_byte(s, i, b)) {
            return qjs::Result<platform::Color4>::fail(qjs::ErrorInfo{"invalid rgb()", {}, {}});
        }
        return qjs::Result<platform::Color4>::ok(rgba255(r, g, b));
    }

    return qjs::Result<platform::Color4>::fail(qjs::ErrorInfo{"unsupported color format", {}, {}});
}

qjs::Result<platform::Color4> parse_color(const qjs::Value& v) {
    if (auto s = v.toString(); s.success) {
        return parse_color_string(std::move(s.value));
    }
    return qjs::Result<platform::Color4>::fail(qjs::ErrorInfo{"color must be a string", {}, {}});
}

} // namespace qianjs::canvas
