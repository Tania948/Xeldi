#pragma once
// Matrices de camara, rayos de pantalla y navegacion estilo Blender.
#include "core/Camera.hpp"

namespace xeldi {

struct CameraMatrices {
    Mat4 view, proj, viewProj;
    Vec3 eye, forward, right, up;
    bool ortho = false;
    float nearPlane = 0.1f, farPlane = 2000.0f;
};

struct Ray {
    Vec3 origin;
    Vec3 dir;  ///< normalizado
    Vec3 at(float t) const { return origin + dir * t; }
};

enum class ViewPreset { Front, Back, Right, Left, Top, Iso };

CameraMatrices cameraMatrices(const CameraState& cam, int width, int height);
/// Rayo que sale de la camara por el pixel de pantalla (sx, sy).
Ray screenRay(const CameraState& cam, int width, int height, float sx, float sy);
/// Proyecta un punto a pantalla. false si queda detras de la camara.
bool projectToScreen(const CameraMatrices& m, Vec3 p, int width, int height, float& sx, float& sy, float& depth);

void orbitCamera(CameraState& cam, float dYawDeg, float dPitchDeg);
/// Desplaza el objetivo segun pixeles de pantalla.
void panCamera(CameraState& cam, float dxPx, float dyPx, int viewportHeight);
void zoomCamera(CameraState& cam, float factor);
void applyViewPreset(CameraState& cam, ViewPreset preset);
/// Altura visible (unidades del mundo) en el punto objetivo.
float visibleHeight(const CameraState& cam);

}  // namespace xeldi
