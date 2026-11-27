#include "engine2d/Compositor.hpp"

#include <algorithm>
#include <cmath>

namespace xeldi {

static float blendChannel(BlendMode m, float cb, float cs) {
    switch (m) {
        case BlendMode::Normal: return cs;
        case BlendMode::Multiply: return cb * cs;
        case BlendMode::Screen: return cb + cs - cb * cs;
        case BlendMode::Add: return std::min(1.0f, cb + cs);
        case BlendMode::Darken: return std::min(cb, cs);
        case BlendMode::Lighten: return std::max(cb, cs);
        case BlendMode::Overlay:
            return cb <= 0.5f ? 2 * cb * cs : 1 - 2 * (1 - cb) * (1 - cs);
    }
    return cs;
}

Rgba blendPixel(Rgba dst, Rgba src, BlendMode mode, float opacity) {
    float as = alpha(src) / 255.0f * std::clamp(opacity, 0.0f, 1.0f);
    if (as <= 0.0f) return dst;
    float ab = alpha(dst) / 255.0f;
    if (ab <= 0.0f && mode == BlendMode::Normal && as >= 1.0f) return src;
    float cs[3] = {red(src) / 255.0f, green(src) / 255.0f, blue(src) / 255.0f};
    float cb[3] = {red(dst) / 255.0f, green(dst) / 255.0f, blue(dst) / 255.0f};
    float ao = as + ab * (1 - as);
    std::uint8_t out[3];
    for (int i = 0; i < 3; ++i) {
        float mixed = (1 - ab) * cs[i] + ab * blendChannel(mode, cb[i], cs[i]);
        float co = (as * mixed + ab * cb[i] * (1 - as)) / ao;
        out[i] = std::uint8_t(std::clamp(co * 255.0f + 0.5f, 0.0f, 255.0f));
    }
    return makeRgba(out[0], out[1], out[2], std::uint8_t(std::clamp(ao * 255.0f + 0.5f, 0.0f, 255.0f)));
}

void composeImage(Image& out, const Image& src, BlendMode mode, float opacity) {
    int w = std::min(out.width(), src.width()), h = std::min(out.height(), src.height());
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            Rgba s = src.pixel(x, y);
            if (alpha(s)) out.setPixel(x, y, blendPixel(out.pixel(x, y), s, mode, opacity));
        }
}

Image composeFrame(const Document& doc, int frame, const ComposeOptions& opt) {
    Image out(doc.width, doc.height, opt.background);
    for (int l = 0; l < int(doc.layers.size()); ++l) {
        if (opt.onlyLayer >= 0 && l != opt.onlyLayer) continue;
        const Layer& layer = doc.layers[l];
        if (layer.type != LayerType::Raster2D) continue;
        if (!layer.visible && !opt.includeHidden) continue;
        if (const Image* c = doc.cel(l, frame)) composeImage(out, *c, layer.blend, layer.opacity);
    }
    return out;
}

}  // namespace xeldi
