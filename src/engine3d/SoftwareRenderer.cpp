#include "engine3d/SoftwareRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "engine2d/Compositor.hpp"

namespace xeldi {

namespace {
struct FaceDef {
    IVec3 n;
    Vec3 corners[4];  // relativas a la esquina minima del voxel
    float light;
};
// Orden antihorario visto desde fuera.
const FaceDef kFaces[6] = {
    {{1, 0, 0}, {{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}, 0.80f},
    {{-1, 0, 0}, {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}, 0.70f},
    {{0, 1, 0}, {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}, 1.00f},
    {{0, -1, 0}, {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}, 0.55f},
    {{0, 0, 1}, {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}, 0.90f},
    {{0, 0, -1}, {{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}, 0.65f},
};
}  // namespace

bool SoftwareRenderer::toScreen(Vec3 p, Vec3& s) const {
    Vec4 c = m_.viewProj.transform(p);
    if (c.w <= 1e-4f) return false;
    s.x = (c.x / c.w * 0.5f + 0.5f) * float(w_);
    s.y = (1.0f - (c.y / c.w * 0.5f + 0.5f)) * float(h_);
    s.z = c.z / c.w;
    return s.z >= -1.0001f && s.z <= 1.0001f;
}

void SoftwareRenderer::fillTriangle(const Vec3 s[3], Rgba color, float opacity, bool writeDepth) {
    float minX = std::min({s[0].x, s[1].x, s[2].x}), maxX = std::max({s[0].x, s[1].x, s[2].x});
    float minY = std::min({s[0].y, s[1].y, s[2].y}), maxY = std::max({s[0].y, s[1].y, s[2].y});
    int x0 = std::max(0, int(std::floor(minX))), x1 = std::min(w_ - 1, int(std::ceil(maxX)));
    int y0 = std::max(0, int(std::floor(minY))), y1 = std::min(h_ - 1, int(std::ceil(maxY)));
    if (x0 > x1 || y0 > y1) return;
    auto edge = [](const Vec3& a, const Vec3& b, float px, float py) {
        return (b.x - a.x) * (py - a.y) - (b.y - a.y) * (px - a.x);
    };
    float area = edge(s[0], s[1], s[2].x, s[2].y);
    if (std::fabs(area) < 1e-6f) return;
    float inv = 1.0f / area;
    for (int y = y0; y <= y1; ++y) {
        float py = y + 0.5f;
        for (int x = x0; x <= x1; ++x) {
            float px = x + 0.5f;
            float w0 = edge(s[1], s[2], px, py) * inv;
            float w1 = edge(s[2], s[0], px, py) * inv;
            float w2 = edge(s[0], s[1], px, py) * inv;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;
            float z = w0 * s[0].z + w1 * s[1].z + w2 * s[2].z;
            float& d = depth_[std::size_t(y) * w_ + x];
            if (z >= d) continue;
            if (writeDepth) d = z;
            if (opacity >= 1.0f)
                color_.setPixel(x, y, color);
            else
                color_.setPixel(x, y, blendPixel(color_.pixel(x, y), color, BlendMode::Normal, opacity));
        }
    }
}

void SoftwareRenderer::fillQuad(const Vec3 q[4], Rgba color, float opacity, bool writeDepth) {
    Vec3 s[4];
    for (int i = 0; i < 4; ++i)
        if (!toScreen(q[i], s[i])) return;  // recorte simple: se descarta si cruza el plano cercano
    Vec3 t1[3] = {s[0], s[1], s[2]};
    Vec3 t2[3] = {s[0], s[2], s[3]};
    fillTriangle(t1, color, opacity, writeDepth);
    fillTriangle(t2, color, opacity, writeDepth);
}

void SoftwareRenderer::drawLine3D(Vec3 a, Vec3 b, Rgba color, bool depthTest) {
    Vec4 ca = m_.viewProj.transform(a), cb = m_.viewProj.transform(b);
    const float eps = 1e-3f;
    if (ca.w < eps && cb.w < eps) return;
    // Recorte contra w = eps (detras de la camara).
    auto lerp4 = [](Vec4 p, Vec4 q, float t) {
        return Vec4{p.x + (q.x - p.x) * t, p.y + (q.y - p.y) * t, p.z + (q.z - p.z) * t, p.w + (q.w - p.w) * t};
    };
    if (ca.w < eps) ca = lerp4(ca, cb, (eps - ca.w) / (cb.w - ca.w));
    if (cb.w < eps) cb = lerp4(cb, ca, (eps - cb.w) / (ca.w - cb.w));
    auto scr = [&](Vec4 c) {
        return Vec3{(c.x / c.w * 0.5f + 0.5f) * float(w_), (1.0f - (c.y / c.w * 0.5f + 0.5f)) * float(h_), c.z / c.w};
    };
    Vec3 sa = scr(ca), sb = scr(cb);
    float dx = sb.x - sa.x, dy = sb.y - sa.y;
    int steps = int(std::ceil(std::max(std::fabs(dx), std::fabs(dy))));
    if (steps > 8192) return;
    for (int i = 0; i <= steps; ++i) {
        float t = steps ? float(i) / float(steps) : 0.0f;
        int x = int(std::floor(sa.x + dx * t)), y = int(std::floor(sa.y + dy * t));
        if (x < 0 || y < 0 || x >= w_ || y >= h_) continue;
        float z = sa.z + (sb.z - sa.z) * t;
        float& d = depth_[std::size_t(y) * w_ + x];
        if (depthTest && z > d + 2e-4f) continue;
        color_.setPixel(x, y, color);
    }
}

void SoftwareRenderer::drawWireCube(IVec3 c, Rgba color) {
    Vec3 o = c.toVec3();
    Vec3 v[8];
    for (int i = 0; i < 8; ++i) v[i] = o + Vec3{float(i & 1), float((i >> 1) & 1), float((i >> 2) & 1)};
    const int e[12][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3}, {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
    for (auto& ed : e) drawLine3D(v[ed[0]], v[ed[1]], color, false);
}

void SoftwareRenderer::drawVoxels(const VoxelGrid& grid, bool shading) {
    grid.forEach([&](IVec3 p, const Voxel& v) {
        Vec3 base = p.toVec3();
        for (const FaceDef& f : kFaces) {
            if (grid.has(p + f.n)) continue;
            Vec3 n = f.n.toVec3();
            Vec3 center = base + Vec3{0.5f, 0.5f, 0.5f} + n * 0.5f;
            Vec3 toEye = m_.ortho ? -m_.forward : m_.eye - center;
            if (dot(n, toEye) <= 0) continue;  // cara trasera
            Vec3 q[4];
            for (int i = 0; i < 4; ++i) q[i] = base + f.corners[i];
            fillQuad(q, shading ? shade(withAlpha(v.rgba, 255), f.light) : withAlpha(v.rgba, 255));
        }
    });
}

void SoftwareRenderer::drawWorkplane(const Workplane& p, bool active, const RenderOptions& opt) {
    float op = active ? 1.0f : opt.inactivePlaneOpacity;
    Vec3 lift = p.normal * 0.01f;
    for (int py = 0; py < p.pixels.height(); ++py)
        for (int px = 0; px < p.pixels.width(); ++px) {
            Rgba c = p.pixels.pixel(px, py);
            if (!alpha(c)) continue;
            float u = float(px), v = float(p.heightPx - 1 - py);
            Vec3 q[4] = {p.pointAt(u, v) + lift, p.pointAt(u + 1, v) + lift, p.pointAt(u + 1, v + 1) + lift,
                         p.pointAt(u, v + 1) + lift};
            fillQuad(q, withAlpha(c, 255), op * (alpha(c) / 255.0f), active);
        }
    Rgba frame = active ? opt.activePlaneFrameColor : opt.planeFrameColor;
    float W = float(p.widthPx), H = float(p.heightPx);
    if (active) {
        // Rejilla de pixeles del plano activo (cada 1 o cada 4 segun tamano).
        int step = std::max(p.widthPx, p.heightPx) > 64 ? 4 : 1;
        Rgba gc = lerpColor(opt.background, frame, 0.22f);
        for (int u = step; u < p.widthPx; u += step) drawLine3D(p.pointAt(float(u), 0) + lift, p.pointAt(float(u), H) + lift, gc, true);
        for (int v = step; v < p.heightPx; v += step) drawLine3D(p.pointAt(0, float(v)) + lift, p.pointAt(W, float(v)) + lift, gc);
    }
    Vec3 c[4] = {p.pointAt(0, 0), p.pointAt(W, 0), p.pointAt(W, H), p.pointAt(0, H)};
    for (int i = 0; i < 4; ++i) drawLine3D(c[i] + lift, c[(i + 1) % 4] + lift, frame, !active);
}

void SoftwareRenderer::drawGrid(const RenderOptions& opt) {
    int n = opt.gridHalfExtent;
    for (int i = -n; i <= n; ++i) {
        Rgba c = (i % 8 == 0) ? opt.gridMajorColor : opt.gridColor;
        if (opt.showAxes && i == 0) continue;
        drawLine3D({float(i), 0, float(-n)}, {float(i), 0, float(n)}, c);
        drawLine3D({float(-n), 0, float(i)}, {float(n), 0, float(i)}, c);
    }
    if (opt.showAxes) {
        drawLine3D({float(-n), 0, 0}, {float(n), 0, 0}, 0xE86A92FFu);  // X rosa
        drawLine3D({0, 0, float(-n)}, {0, 0, float(n)}, 0x7FA2F5FFu);  // Z azul
        drawLine3D({0, 0, 0}, {0, float(n) / 2, 0}, 0x8FCFB0FFu);      // Y menta
    }
}

const Image& SoftwareRenderer::render(const Scene3D& scene, const CameraState& cam, int w, int h,
                                      const RenderOptions& opt) {
    w_ = std::max(1, w);
    h_ = std::max(1, h);
    if (color_.width() != w_ || color_.height() != h_) color_ = Image(w_, h_);
    color_.fill(opt.background);
    depth_.assign(std::size_t(w_) * h_, std::numeric_limits<float>::infinity());
    m_ = cameraMatrices(cam, w_, h_);

    drawVoxels(scene.voxels, opt.shading);
    if (opt.showWorkplanes) {
        for (int i = 0; i < int(scene.workplanes.size()); ++i)
            if (i == scene.activeWorkplane && scene.workplanes[i].visible)
                drawWorkplane(scene.workplanes[i], true, opt);
    }
    if (opt.showGrid) drawGrid(opt);
    if (opt.showWorkplanes) {
        for (int i = 0; i < int(scene.workplanes.size()); ++i)
            if (i != scene.activeWorkplane && scene.workplanes[i].visible)
                drawWorkplane(scene.workplanes[i], false, opt);
    }
    if (opt.hoverCell) drawWireCube(*opt.hoverCell, opt.hoverColor);
    return color_;
}

}  // namespace xeldi
