#include "core/Image.hpp"

#include <algorithm>

namespace xeldi {

Rect Rect::united(const Rect& o) const {
    if (isEmpty()) return o;
    if (o.isEmpty()) return *this;
    int x0 = std::min(x, o.x), y0 = std::min(y, o.y);
    int x1 = std::max(x + w, o.x + o.w), y1 = std::max(y + h, o.y + o.h);
    return {x0, y0, x1 - x0, y1 - y0};
}

Rect Rect::intersected(const Rect& o) const {
    int x0 = std::max(x, o.x), y0 = std::max(y, o.y);
    int x1 = std::min(x + w, o.x + o.w), y1 = std::min(y + h, o.y + o.h);
    if (x1 <= x0 || y1 <= y0) return {};
    return {x0, y0, x1 - x0, y1 - y0};
}

Image::Image(int w, int h, Rgba fill) : w_(std::max(0, w)), h_(std::max(0, h)) {
    px_.assign(std::size_t(w_) * h_, fill);
}

void Image::fill(Rgba c) { std::fill(px_.begin(), px_.end(), c); }

bool Image::isFullyTransparent() const {
    return std::all_of(px_.begin(), px_.end(), [](Rgba c) { return alpha(c) == 0; });
}

Rect Image::opaqueBounds() const {
    int x0 = w_, y0 = h_, x1 = -1, y1 = -1;
    for (int y = 0; y < h_; ++y)
        for (int x = 0; x < w_; ++x)
            if (alpha(px_[std::size_t(y) * w_ + x])) {
                x0 = std::min(x0, x); y0 = std::min(y0, y);
                x1 = std::max(x1, x); y1 = std::max(y1, y);
            }
    if (x1 < 0) return {};
    return {x0, y0, x1 - x0 + 1, y1 - y0 + 1};
}

Image Image::copy(const Rect& r) const {
    Image out(r.w, r.h);
    for (int y = 0; y < r.h; ++y)
        for (int x = 0; x < r.w; ++x) out.setPixel(x, y, pixel(r.x + x, r.y + y));
    return out;
}

void Image::paste(const Image& src, int dx, int dy) {
    for (int y = 0; y < src.height(); ++y)
        for (int x = 0; x < src.width(); ++x) setPixel(dx + x, dy + y, src.pixel(x, y));
}

Image Image::scaledNearest(int nw, int nh) const {
    Image out(nw, nh);
    if (isNull()) return out;
    for (int y = 0; y < nh; ++y)
        for (int x = 0; x < nw; ++x)
            out.setPixel(x, y, pixel(int(std::int64_t(x) * w_ / nw), int(std::int64_t(y) * h_ / nh)));
    return out;
}

std::vector<std::uint8_t> Image::toRgba8() const {
    std::vector<std::uint8_t> out(px_.size() * 4);
    for (std::size_t i = 0; i < px_.size(); ++i) {
        Rgba c = px_[i];
        out[i * 4 + 0] = red(c);
        out[i * 4 + 1] = green(c);
        out[i * 4 + 2] = blue(c);
        out[i * 4 + 3] = alpha(c);
    }
    return out;
}

Image Image::fromRgba8(int w, int h, const std::uint8_t* b) {
    Image img(w, h);
    for (std::size_t i = 0; i < img.px_.size(); ++i)
        img.px_[i] = makeRgba(b[i * 4], b[i * 4 + 1], b[i * 4 + 2], b[i * 4 + 3]);
    return img;
}

}  // namespace xeldi
