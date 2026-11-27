#include "core/Workplane.hpp"

#include <cmath>

namespace xeldi {

Workplane Workplane::axisAligned(PlaneAxis axis, int offset, int size, std::string id, std::string name) {
    Workplane w;
    w.id = std::move(id);
    w.name = std::move(name);
    w.widthPx = w.heightPx = size;
    float h = float(size / 2);
    switch (axis) {
        case PlaneAxis::Front:  // plano XY mirando hacia +Z
            w.normal = {0, 0, 1}; w.right = {1, 0, 0}; w.up = {0, 1, 0};
            w.origin = {-h, 0, float(offset)};
            break;
        case PlaneAxis::Side:  // plano ZY mirando hacia +X (derecha de la pantalla = -Z)
            w.normal = {1, 0, 0}; w.right = {0, 0, -1}; w.up = {0, 1, 0};
            w.origin = {float(offset), 0, h};
            break;
        case PlaneAxis::Top:  // plano XZ mirando hacia +Y (arriba de la imagen = -Z)
            w.normal = {0, 1, 0}; w.right = {1, 0, 0}; w.up = {0, 0, -1};
            w.origin = {-h, float(offset), h};
            break;
    }
    w.ensurePixels();
    return w;
}

void Workplane::ensurePixels() {
    if (pixels.width() != widthPx || pixels.height() != heightPx) {
        Image n(widthPx, heightPx);
        n.paste(pixels, 0, 0);
        pixels = std::move(n);
    }
}

Vec3 Workplane::pixelCenter(int px, int py) const {
    float u = float(px) + 0.5f;
    float v = float(heightPx - 1 - py) + 0.5f;
    return pointAt(u, v);
}

std::optional<PlanePixel> Workplane::worldToPixel(Vec3 p) const {
    Vec3 d = p - origin;
    float u = dot(d, right) / dot(right, right);
    float v = dot(d, up) / dot(up, up);
    int iu = int(std::floor(u));
    int iv = int(std::floor(v));
    if (iu < 0 || iv < 0 || iu >= widthPx || iv >= heightPx) return std::nullopt;
    return PlanePixel{iu, heightPx - 1 - iv, p};
}

}  // namespace xeldi
