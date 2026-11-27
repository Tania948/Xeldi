#include "engine3d/ViewProjection.hpp"

#include <climits>
#include <vector>

namespace xeldi {

const char* toString(OrthoView v) {
    switch (v) {
        case OrthoView::Front: return "front";
        case OrthoView::Back: return "back";
        case OrthoView::Right: return "right";
        case OrthoView::Left: return "left";
        case OrthoView::Top: return "top";
    }
    return "front";
}

CanvasCell worldToCanvas(OrthoView v, IVec3 p, int w, int h) {
    int hw = w / 2, hd = w / 2, hh = h / 2;
    switch (v) {
        case OrthoView::Front: return {p.x + hw, h - 1 - p.y, p.z};
        case OrthoView::Back: return {hw - 1 - p.x, h - 1 - p.y, -p.z};
        case OrthoView::Right: return {hd - 1 - p.z, h - 1 - p.y, p.x};
        case OrthoView::Left: return {p.z + hd, h - 1 - p.y, -p.x};
        case OrthoView::Top: return {p.x + hw, p.z + hh, p.y};
    }
    return {};
}

Image projectVoxels(const VoxelGrid& grid, OrthoView view, int w, int h, bool shadeByDepth) {
    Image out(w, h);
    std::vector<int> depth(std::size_t(w) * h, INT_MIN);
    int minD = INT_MAX, maxD = INT_MIN;
    grid.forEach([&](IVec3 p, const Voxel& v) {
        CanvasCell c = worldToCanvas(view, p, w, h);
        if (c.col < 0 || c.row < 0 || c.col >= w || c.row >= h) return;
        int& d = depth[std::size_t(c.row) * w + c.col];
        if (c.depth > d) {
            d = c.depth;
            out.setPixel(c.col, c.row, v.rgba);
        }
        minD = std::min(minD, c.depth);
        maxD = std::max(maxD, c.depth);
    });
    if (shadeByDepth && maxD > minD) {
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                int d = depth[std::size_t(y) * w + x];
                if (d == INT_MIN) continue;
                float t = float(d - minD) / float(maxD - minD);
                out.setPixel(x, y, shade(out.pixel(x, y), 0.7f + 0.3f * t));
            }
    }
    return out;
}

}  // namespace xeldi
