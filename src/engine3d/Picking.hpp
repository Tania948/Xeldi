#pragma once
#include <optional>

#include "core/Document.hpp"
#include "engine3d/CameraMath.hpp"

namespace xeldi {

struct VoxelHit {
    IVec3 cell;    ///< voxel impactado
    IVec3 normal;  ///< cara impactada (la celda vecina es cell + normal)
    float t = 0;
    IVec3 adjacent() const { return cell + normal; }
};

/// Recorrido DDA (Amanatides & Woo) por la rejilla dispersa.
std::optional<VoxelHit> raycastVoxels(const VoxelGrid& grid, const Ray& ray, float maxDistance = 4000.0f);
/// Interseccion rayo-plano. Devuelve t >= 0.
std::optional<float> intersectPlane(Vec3 planePoint, Vec3 planeNormal, const Ray& ray);
/// Pixel del plano de trabajo bajo el rayo.
std::optional<PlanePixel> pickWorkplane(const Workplane& plane, const Ray& ray);
/// Celda del mundo donde el rayo cruza el plano (para colocar voxeles sobre un plano).
/// La celda queda del lado de la normal del plano.
std::optional<IVec3> cellOnPlane(const Workplane& plane, const Ray& ray);

}  // namespace xeldi
