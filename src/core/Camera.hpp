#pragma once
#include "core/Math.hpp"

namespace xeldi {

/// Estado serializable de la camara orbital (tipo Blender).
struct CameraState {
    Vec3 target{0, 8, 0};
    float yawDeg = 35.0f;    ///< giro alrededor de Y
    float pitchDeg = 28.0f;  ///< elevacion
    float distance = 56.0f;
    float fovDeg = 45.0f;
    bool ortho = false;

    Vec3 position() const;
};

}  // namespace xeldi
