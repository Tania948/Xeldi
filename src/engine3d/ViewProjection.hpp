#pragma once
// Proyeccion ortografica exacta voxel -> pixel (1 voxel = 1 pixel), para
// convertir modelos 3D en sprites o frames de animacion.
//
// Convencion lienzo <-> mundo para un lienzo W x H (profundidad D = W):
//   Front  (camara en +Z): col = x + W/2,       fila = H-1-y
//   Back   (camara en -Z): col = W/2-1-x,       fila = H-1-y
//   Right  (camara en +X): col = D/2-1-z,       fila = H-1-y
//   Left   (camara en -X): col = z + D/2,       fila = H-1-y
//   Top    (camara en +Y): col = x + W/2,       fila = z + H/2
#include "core/Document.hpp"

namespace xeldi {

enum class OrthoView { Front, Back, Right, Left, Top };

const char* toString(OrthoView v);

struct CanvasCell {
    int col = 0, row = 0;
    int depth = 0;  ///< mayor = mas cerca de la camara
};

CanvasCell worldToCanvas(OrthoView v, IVec3 p, int w, int h);

/// Proyecta los voxeles a una imagen W x H. Con `shadeByDepth` oscurece
/// ligeramente lo que esta mas lejos (util como referencia).
Image projectVoxels(const VoxelGrid& grid, OrthoView view, int w, int h, bool shadeByDepth = false);

}  // namespace xeldi
