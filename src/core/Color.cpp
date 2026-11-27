#include "core/Color.hpp"

#include <algorithm>
#include <cstdio>

namespace xeldi {

std::string toHex(Rgba c) {
    char buf[10];
    std::snprintf(buf, sizeof buf, "#%02X%02X%02X%02X", red(c), green(c), blue(c), alpha(c));
    return buf;
}

static int hexDigit(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

std::optional<Rgba> fromHex(std::string_view s) {
    if (!s.empty() && s.front() == '#') s.remove_prefix(1);
    std::uint32_t v = 0;
    for (char ch : s) {
        int d = hexDigit(ch);
        if (d < 0) return std::nullopt;
        v = (v << 4) | std::uint32_t(d);
    }
    switch (s.size()) {
        case 3: {
            auto r = std::uint8_t(((v >> 8) & 0xF) * 17);
            auto g = std::uint8_t(((v >> 4) & 0xF) * 17);
            auto b = std::uint8_t((v & 0xF) * 17);
            return makeRgba(r, g, b);
        }
        case 6: return (v << 8) | 0xFFu;
        case 8: return v;
        default: return std::nullopt;
    }
}

Rgba lerpColor(Rgba a, Rgba b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto mix = [t](std::uint8_t x, std::uint8_t y) {
        return std::uint8_t(float(x) + (float(y) - float(x)) * t + 0.5f);
    };
    return makeRgba(mix(red(a), red(b)), mix(green(a), green(b)), mix(blue(a), blue(b)),
                    mix(alpha(a), alpha(b)));
}

Rgba shade(Rgba c, float factor) {
    auto f = [factor](std::uint8_t x) {
        return std::uint8_t(std::clamp(float(x) * factor + 0.5f, 0.0f, 255.0f));
    };
    return makeRgba(f(red(c)), f(green(c)), f(blue(c)), alpha(c));
}

}  // namespace xeldi
