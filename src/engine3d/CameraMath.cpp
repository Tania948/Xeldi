#include "engine3d/CameraMath.hpp"

#include <algorithm>
#include <cmath>

namespace xeldi {

float visibleHeight(const CameraState& cam) {
    return 2.0f * cam.distance * std::tan(radians(cam.fovDeg) / 2.0f);
}

CameraMatrices cameraMatrices(const CameraState& cam, int w, int h) {
    CameraMatrices m;
    m.eye = cam.position();
    m.forward = normalize(cam.target - m.eye);
    Vec3 worldUp{0, 1, 0};
    if (std::fabs(dot(m.forward, worldUp)) > 0.999f) worldUp = {0, 0, -1};
    m.right = normalize(cross(m.forward, worldUp));
    m.up = cross(m.right, m.forward);
    m.view = Mat4::lookAt(m.eye, cam.target, m.up);
    float aspect = h > 0 ? float(w) / float(h) : 1.0f;
    m.ortho = cam.ortho;
    m.nearPlane = cam.ortho ? -2000.0f : std::max(0.05f, cam.distance * 0.01f);
    m.farPlane = 4000.0f;
    if (cam.ortho) {
        float hh = visibleHeight(cam) / 2, hw = hh * aspect;
        m.proj = Mat4::ortho(-hw, hw, -hh, hh, m.nearPlane, m.farPlane);
    } else {
        m.proj = Mat4::perspective(radians(cam.fovDeg), aspect, m.nearPlane, m.farPlane);
    }
    m.viewProj = m.proj * m.view;
    return m;
}

Ray screenRay(const CameraState& cam, int w, int h, float sx, float sy) {
    CameraMatrices m = cameraMatrices(cam, w, h);
    float aspect = h > 0 ? float(w) / float(h) : 1.0f;
    float nx = 2.0f * sx / float(std::max(1, w)) - 1.0f;
    float ny = 1.0f - 2.0f * sy / float(std::max(1, h));
    if (cam.ortho) {
        float hh = visibleHeight(cam) / 2, hw = hh * aspect;
        Vec3 o = m.eye + m.right * (nx * hw) + m.up * (ny * hh) - m.forward * 1000.0f;
        return {o, m.forward};
    }
    float t = std::tan(radians(cam.fovDeg) / 2);
    Vec3 d = m.forward + m.right * (nx * t * aspect) + m.up * (ny * t);
    return {m.eye, normalize(d)};
}

bool projectToScreen(const CameraMatrices& m, Vec3 p, int w, int h, float& sx, float& sy, float& depth) {
    Vec4 c = m.viewProj.transform(p);
    if (c.w <= 1e-5f) return false;
    float ix = c.x / c.w, iy = c.y / c.w;
    sx = (ix * 0.5f + 0.5f) * float(w);
    sy = (1.0f - (iy * 0.5f + 0.5f)) * float(h);
    depth = c.z / c.w;
    return true;
}

void orbitCamera(CameraState& cam, float dYaw, float dPitch) {
    cam.yawDeg = std::fmod(cam.yawDeg + dYaw, 360.0f);
    cam.pitchDeg = std::clamp(cam.pitchDeg + dPitch, -89.5f, 89.5f);
}

void panCamera(CameraState& cam, float dx, float dy, int vh) {
    CameraMatrices m = cameraMatrices(cam, 1, 1);
    float unitsPerPx = visibleHeight(cam) / float(std::max(1, vh));
    cam.target += m.right * (-dx * unitsPerPx) + m.up * (dy * unitsPerPx);
}

void zoomCamera(CameraState& cam, float factor) {
    cam.distance = std::clamp(cam.distance * factor, 2.0f, 2000.0f);
}

void applyViewPreset(CameraState& cam, ViewPreset p) {
    switch (p) {
        case ViewPreset::Front: cam.yawDeg = 0; cam.pitchDeg = 0; cam.ortho = true; break;
        case ViewPreset::Back: cam.yawDeg = 180; cam.pitchDeg = 0; cam.ortho = true; break;
        case ViewPreset::Right: cam.yawDeg = 90; cam.pitchDeg = 0; cam.ortho = true; break;
        case ViewPreset::Left: cam.yawDeg = -90; cam.pitchDeg = 0; cam.ortho = true; break;
        case ViewPreset::Top: cam.yawDeg = 0; cam.pitchDeg = 89.5f; cam.ortho = true; break;
        case ViewPreset::Iso: cam.yawDeg = 45; cam.pitchDeg = 30; cam.ortho = true; break;
    }
}

}  // namespace xeldi
