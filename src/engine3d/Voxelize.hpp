#pragma once
// Puentes entre el mundo plano y el volumetrico:
//  * extrusion de un plano de trabajo a voxeles
//  * "generar desde vistas": reconstruye un objeto simple a partir de sus
//    vistas ortograficas frontal / lateral / superior (casco visual).
#include <functional>
#include <vector>

#include "core/Document.hpp"

namespace xeldi {

using VoxelSink = std::function<void(IVec3, Voxel)>;

/// Extruye los pixeles opacos del plano `depth` pasos a lo largo de su normal.
/// Para planos inclinados rellena huecos con Bresenham 3D entre pixeles vecinos.
void extrudeWorkplane(const Workplane& plane, int depth, const VoxelSink& sink);

/// Linea 3D entera (Bresenham 3D) entre dos celdas, incluyendo extremos.
std::vector<IVec3> bresenham3D(IVec3 a, IVec3 b);

struct ViewSet {
    const Image* front = nullptr;  ///< vista desde +Z (obligatoria)
    const Image* side = nullptr;   ///< vista desde +X (derecha); nullptr = sin restriccion
    const Image* top = nullptr;    ///< vista desde +Y; nullptr = sin restriccion
};

/// Genera voxeles que, vistos desde cada eje, reproducen las siluetas dadas.
/// Las imagenes usan la convencion de lienzo de Xeldi (ver ViewProjection.hpp).
VoxelGrid generateFromViews(const ViewSet& views);

}  // namespace xeldi
