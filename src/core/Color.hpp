#pragma once
// Colores RGBA8 empaquetados como 0xRRGGBBAA. Alfa 0 = transparente.
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace xeldi {

using Rgba = std::uint32_t;

constexpr Rgba kTransparent = 0;

constexpr Rgba makeRgba(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) {
    return (Rgba(r) << 24) | (Rgba(g) << 16) | (Rgba(b) << 8) | Rgba(a);
}
constexpr std::uint8_t red(Rgba c) { return std::uint8_t(c >> 24); }
constexpr std::uint8_t green(Rgba c) { return std::uint8_t((c >> 16) & 0xFF); }
constexpr std::uint8_t blue(Rgba c) { return std::uint8_t((c >> 8) & 0xFF); }
constexpr std::uint8_t alpha(Rgba c) { return std::uint8_t(c & 0xFF); }
constexpr Rgba withAlpha(Rgba c, std::uint8_t a) { return (c & 0xFFFFFF00u) | a; }
constexpr bool isTransparent(Rgba c) { return alpha(c) == 0; }

/// "#RRGGBBAA"
std::string toHex(Rgba c);
/// Acepta "#RGB", "#RRGGBB" y "#RRGGBBAA" (el # es opcional).
std::optional<Rgba> fromHex(std::string_view s);

/// Interpolacion lineal por canal (t en [0,1]).
Rgba lerpColor(Rgba a, Rgba b, float t);
/// Multiplica el brillo RGB por un factor (alfa intacto).
Rgba shade(Rgba c, float factor);

}  // namespace xeldi
