#pragma once
// Bitmap RGBA8 simple. Fila 0 = arriba. Sin dependencias externas.
#include <cstdint>
#include <vector>

#include "core/Color.hpp"

namespace xeldi {

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;
    bool isEmpty() const { return w <= 0 || h <= 0; }
    bool contains(int px, int py) const { return px >= x && py >= y && px < x + w && py < y + h; }
    Rect united(const Rect& o) const;
    Rect intersected(const Rect& o) const;
    bool operator==(const Rect&) const = default;
};

class Image {
public:
    Image() = default;
    Image(int w, int h, Rgba fill = kTransparent);

    int width() const { return w_; }
    int height() const { return h_; }
    bool isNull() const { return w_ <= 0 || h_ <= 0; }
    Rect rect() const { return {0, 0, w_, h_}; }
    bool contains(int x, int y) const { return x >= 0 && y >= 0 && x < w_ && y < h_; }

    Rgba pixel(int x, int y) const { return contains(x, y) ? px_[std::size_t(y) * w_ + x] : kTransparent; }
    void setPixel(int x, int y, Rgba c) {
        if (contains(x, y)) px_[std::size_t(y) * w_ + x] = c;
    }
    Rgba* data() { return px_.data(); }
    const Rgba* data() const { return px_.data(); }
    const std::vector<Rgba>& pixels() const { return px_; }

    void fill(Rgba c);
    /// true si todos los pixeles son transparentes.
    bool isFullyTransparent() const;
    /// Rectangulo minimo con pixeles no transparentes (vacio si no hay).
    Rect opaqueBounds() const;

    Image copy(const Rect& r) const;
    void paste(const Image& src, int dx, int dy);
    Image scaledNearest(int nw, int nh) const;

    /// Bytes R,G,B,A consecutivos (orden de archivo/PNG).
    std::vector<std::uint8_t> toRgba8() const;
    static Image fromRgba8(int w, int h, const std::uint8_t* bytes);

    bool operator==(const Image& o) const { return w_ == o.w_ && h_ == o.h_ && px_ == o.px_; }

private:
    int w_ = 0, h_ = 0;
    std::vector<Rgba> px_;
};

}  // namespace xeldi
