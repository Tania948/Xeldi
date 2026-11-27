#include "engine3d/Voxelize.hpp"

#include <cmath>
#include <cstdlib>

#include "engine3d/ViewProjection.hpp"

namespace xeldi {

std::vector<IVec3> bresenham3D(IVec3 a, IVec3 b) {
    std::vector<IVec3> pts;
    IVec3 d{std::abs(b.x - a.x), std::abs(b.y - a.y), std::abs(b.z - a.z)};
    IVec3 s{b.x > a.x ? 1 : -1, b.y > a.y ? 1 : -1, b.z > a.z ? 1 : -1};
    IVec3 p = a;
    pts.push_back(p);
    if (d.x >= d.y && d.x >= d.z) {
        int e1 = 2 * d.y - d.x, e2 = 2 * d.z - d.x;
        while (p.x != b.x) {
            p.x += s.x;
            if (e1 >= 0) { p.y += s.y; e1 -= 2 * d.x; }
            if (e2 >= 0) { p.z += s.z; e2 -= 2 * d.x; }
            e1 += 2 * d.y; e2 += 2 * d.z;
            pts.push_back(p);
        }
    } else if (d.y >= d.x && d.y >= d.z) {
        int e1 = 2 * d.x - d.y, e2 = 2 * d.z - d.y;
        while (p.y != b.y) {
            p.y += s.y;
            if (e1 >= 0) { p.x += s.x; e1 -= 2 * d.y; }
            if (e2 >= 0) { p.z += s.z; e2 -= 2 * d.y; }
            e1 += 2 * d.x; e2 += 2 * d.z;
            pts.push_back(p);
        }
    } else {
        int e1 = 2 * d.y - d.z, e2 = 2 * d.x - d.z;
        while (p.z != b.z) {
            p.z += s.z;
            if (e1 >= 0) { p.y += s.y; e1 -= 2 * d.z; }
            if (e2 >= 0) { p.x += s.x; e2 -= 2 * d.z; }
            e1 += 2 * d.y; e2 += 2 * d.x;
            pts.push_back(p);
        }
    }
    return pts;
}

void extrudeWorkplane(const Workplane& plane, int depth, const VoxelSink& sink) {
    if (depth == 0) return;
    int dir = depth > 0 ? 1 : -1;
    int n = std::abs(depth);
    const Image& img = plane.pixels;
    auto cellFor = [&](int px, int py, int k) {
        // k = 0 es la capa pegada al plano, del lado de la normal (o del opuesto si depth < 0).
        float off = dir > 0 ? (float(k) + 0.5f) : -(float(k) + 0.5f);
        return floorToCell(plane.pixelCenter(px, py) + plane.normal * off);
    };
    for (int py = 0; py < img.height(); ++py)
        for (int px = 0; px < img.width(); ++px) {
            Rgba c = img.pixel(px, py);
            if (!alpha(c)) continue;
            Voxel v{withAlpha(c, 255), 0};
            for (int k = 0; k < n; ++k) {
                IVec3 cell = cellFor(px, py, k);
                sink(cell, v);
                // Rellena huecos hacia los vecinos derecho e inferior (planos inclinados).
                const int nb[2][2] = {{1, 0}, {0, 1}};
                for (const auto& o : nb) {
                    int qx = px + o[0], qy = py + o[1];
                    if (!alpha(img.pixel(qx, qy))) continue;
                    IVec3 other = cellFor(qx, qy, k);
                    for (const IVec3& g : bresenham3D(cell, other)) sink(g, v);
                }
            }
        }
}

VoxelGrid generateFromViews(const ViewSet& views) {
    VoxelGrid grid;
    if (!views.front) return grid;
    const int w = views.front->width(), h = views.front->height();
    const int d = w;  // profundidad = ancho del lienzo
    auto opaque = [&](const Image* img, OrthoView v, IVec3 p) {
        if (!img) return true;
        CanvasCell c = worldToCanvas(v, p, w, h);
        return alpha(img->pixel(c.col, c.row)) != 0;
    };
    auto colorOf = [&](const Image* img, OrthoView v, IVec3 p) {
        CanvasCell c = worldToCanvas(v, p, w, h);
        return withAlpha(img->pixel(c.col, c.row), 255);
    };
    const int x0 = -w / 2, z0 = -d / 2;
    // 1) Tallado: dentro si todas las vistas disponibles lo cubren.
    std::vector<char> inside(std::size_t(w) * h * d, 0);
    auto idx = [&](int x, int y, int z) { return (std::size_t(z - z0) * h + y) * w + (x - x0); };
    for (int z = z0; z < z0 + d; ++z)
        for (int y = 0; y < h; ++y)
            for (int x = x0; x < x0 + w; ++x) {
                IVec3 p{x, y, z};
                if (opaque(views.front, OrthoView::Front, p) && opaque(views.side, OrthoView::Right, p) &&
                    opaque(views.top, OrthoView::Top, p))
                    inside[idx(x, y, z)] = 1;
            }
    auto in = [&](int x, int y, int z) {
        if (x < x0 || x >= x0 + w || y < 0 || y >= h || z < z0 || z >= z0 + d) return false;
        return inside[idx(x, y, z)] != 0;
    };
    // 2) Color: la vista desde la que la superficie es visible (frente > lado > arriba).
    for (int z = z0; z < z0 + d; ++z)
        for (int y = 0; y < h; ++y)
            for (int x = x0; x < x0 + w; ++x) {
                if (!in(x, y, z)) continue;
                IVec3 p{x, y, z};
                bool frontSurface = true, sideSurface = true, topSurface = true;
                for (int k = z + 1; k < z0 + d && frontSurface; ++k) frontSurface = !in(x, y, k);
                for (int k = x + 1; k < x0 + w && sideSurface; ++k) sideSurface = !in(k, y, z);
                for (int k = y + 1; k < h && topSurface; ++k) topSurface = !in(x, k, z);
                Rgba c;
                if (frontSurface) c = colorOf(views.front, OrthoView::Front, p);
                else if (sideSurface && views.side) c = colorOf(views.side, OrthoView::Right, p);
                else if (topSurface && views.top) c = colorOf(views.top, OrthoView::Top, p);
                else c = colorOf(views.front, OrthoView::Front, p);
                grid.set(p, Voxel{c, 0});
            }
    return grid;
}

}  // namespace xeldi
