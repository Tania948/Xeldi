#include "engine2d/OnionSkin.hpp"

#include <algorithm>
#include <cmath>

#include "engine2d/Compositor.hpp"

namespace xeldi {

std::vector<OnionGhost> onionGhosts(const Document& doc, int frame, const OnionSkinSettings& s,
                                    const AnimationTag* tag) {
    std::vector<OnionGhost> out;
    if (!s.enabled) return out;
    int lo = 0, hi = doc.frameCount() - 1;
    bool wrap = false;
    if (tag && s.loopWithinTag && frame >= tag->from && frame <= tag->to && tag->to > tag->from) {
        lo = tag->from;
        hi = tag->to;
        wrap = true;
    }
    int span = hi - lo + 1;
    auto resolve = [&](int f) -> int {
        if (f >= lo && f <= hi) return f;
        if (!wrap) return -1;
        return lo + ((f - lo) % span + span) % span;
    };
    auto add = [&](int d, bool past) {
        int f = resolve(past ? frame - d : frame + d);
        if (f < 0 || f == frame) return;
        for (const auto& g : out)
            if (g.frame == f) return;  // ya incluido (tags cortos)
        float op = s.baseOpacity * std::pow(s.falloff, float(d - 1));
        out.push_back({f, d, past, op, past ? s.prevTint : s.nextTint});
    };
    int maxD = std::max(s.prevFrames, s.nextFrames);
    for (int d = 1; d <= maxD; ++d) {
        if (d <= s.prevFrames) add(d, true);
        if (d <= s.nextFrames) add(d, false);
    }
    return out;
}

Rgba tintGhostPixel(Rgba c, Rgba tint, float opacity) {
    Rgba mixed = lerpColor(withAlpha(c, 255), withAlpha(tint, 255), 0.55f);
    float a = alpha(c) * std::clamp(opacity, 0.0f, 1.0f);
    return withAlpha(mixed, std::uint8_t(std::clamp(a + 0.5f, 0.0f, 255.0f)));
}

void renderOnion(const Document& doc, int frame, const OnionSkinSettings& s, int currentLayer, Image& out,
                 const AnimationTag* loopTag) {
    auto ghosts = onionGhosts(doc, frame, s, loopTag);
    // Los mas lejanos primero para que los cercanos queden encima.
    std::sort(ghosts.begin(), ghosts.end(), [](const OnionGhost& a, const OnionGhost& b) {
        return a.distance > b.distance;
    });
    ComposeOptions co;
    if (s.onlyCurrentLayer) co.onlyLayer = currentLayer;
    for (const auto& g : ghosts) {
        Image src = composeFrame(doc, g.frame, co);
        for (int y = 0; y < src.height(); ++y)
            for (int x = 0; x < src.width(); ++x) {
                Rgba c = src.pixel(x, y);
                if (alpha(c)) out.setPixel(x, y, blendPixel(out.pixel(x, y), tintGhostPixel(c, g.tint, g.opacity),
                                                            BlendMode::Normal));
            }
    }
}

}  // namespace xeldi
