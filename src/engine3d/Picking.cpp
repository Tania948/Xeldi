#include "engine3d/Picking.hpp"

#include <cmath>
#include <limits>

namespace xeldi {

static bool rayBox(const Ray& r, Vec3 mn, Vec3 mx, float& tEnter, float& tExit) {
    tEnter = -std::numeric_limits<float>::infinity();
    tExit = std::numeric_limits<float>::infinity();
    for (int i = 0; i < 3; ++i) {
        float o = r.origin[i], d = r.dir[i], lo = mn[i], hi = mx[i];
        if (std::fabs(d) < 1e-9f) {
            if (o < lo || o > hi) return false;
            continue;
        }
        float t1 = (lo - o) / d, t2 = (hi - o) / d;
        if (t1 > t2) std::swap(t1, t2);
        tEnter = std::max(tEnter, t1);
        tExit = std::min(tExit, t2);
        if (tEnter > tExit) return false;
    }
    return tExit >= 0;
}

std::optional<VoxelHit> raycastVoxels(const VoxelGrid& grid, const Ray& ray, float maxDistance) {
    IVec3 mn, mx;
    if (!grid.bounds(mn, mx)) return std::nullopt;
    float tEnter, tExit;
    if (!rayBox(ray, mn.toVec3(), (mx + IVec3{1, 1, 1}).toVec3(), tEnter, tExit)) return std::nullopt;
    float t = std::max(0.0f, tEnter) + 1e-4f;
    Vec3 p = ray.at(t);
    IVec3 cell = floorToCell(p);
    int step[3];
    float tMax[3], tDelta[3];
    IVec3 normal{0, 0, 0};
    int c[3] = {cell.x, cell.y, cell.z};
    for (int i = 0; i < 3; ++i) {
        float d = ray.dir[i];
        if (d > 0) {
            step[i] = 1;
            tMax[i] = t + (float(c[i] + 1) - p[i]) / d;
            tDelta[i] = 1.0f / d;
        } else if (d < 0) {
            step[i] = -1;
            tMax[i] = t + (float(c[i]) - p[i]) / d;
            tDelta[i] = -1.0f / d;
        } else {
            step[i] = 0;
            tMax[i] = tDelta[i] = std::numeric_limits<float>::infinity();
        }
    }
    // Normal de entrada: eje por el que se entro a la caja.
    if (tEnter > 0) {
        int axis = 0;
        float best = -1;
        for (int i = 0; i < 3; ++i) {
            float lo = float(i == 0 ? mn.x : i == 1 ? mn.y : mn.z);
            float hi = float(i == 0 ? mx.x + 1 : i == 1 ? mx.y + 1 : mx.z + 1);
            float d = std::min(std::fabs(p[i] - lo), std::fabs(p[i] - hi));
            if (best < 0 || d < best) { best = d; axis = i; }
        }
        int n[3] = {0, 0, 0};
        n[axis] = -step[axis];
        normal = {n[0], n[1], n[2]};
    }
    float limit = std::min(tExit + 1.0f, maxDistance);
    while (t <= limit) {
        IVec3 cur{c[0], c[1], c[2]};
        if (grid.has(cur)) return VoxelHit{cur, normal, t};
        int axis = (tMax[0] < tMax[1]) ? (tMax[0] < tMax[2] ? 0 : 2) : (tMax[1] < tMax[2] ? 1 : 2);
        t = tMax[axis];
        c[axis] += step[axis];
        tMax[axis] += tDelta[axis];
        int n[3] = {0, 0, 0};
        n[axis] = -step[axis];
        normal = {n[0], n[1], n[2]};
    }
    return std::nullopt;
}

std::optional<float> intersectPlane(Vec3 p0, Vec3 n, const Ray& ray) {
    float denom = dot(n, ray.dir);
    if (std::fabs(denom) < 1e-7f) return std::nullopt;
    float t = dot(p0 - ray.origin, n) / denom;
    if (t < 0) return std::nullopt;
    return t;
}

std::optional<PlanePixel> pickWorkplane(const Workplane& plane, const Ray& ray) {
    auto t = intersectPlane(plane.origin, plane.normal, ray);
    if (!t) return std::nullopt;
    return plane.worldToPixel(ray.at(*t));
}

std::optional<IVec3> cellOnPlane(const Workplane& plane, const Ray& ray) {
    auto t = intersectPlane(plane.origin, plane.normal, ray);
    if (!t) return std::nullopt;
    Vec3 hit = ray.at(*t);
    if (!plane.worldToPixel(hit)) return std::nullopt;
    return floorToCell(hit + plane.normal * 0.5f);
}

}  // namespace xeldi
