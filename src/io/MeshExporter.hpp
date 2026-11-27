#pragma once
// Exporta la rejilla de voxeles como malla OBJ + MTL (una cara por lado
// visible, un material por color). Escala: 1 voxel = 1 unidad.
#include <string>

#include "core/VoxelGrid.hpp"

namespace xeldi::io {

struct MeshStats {
    std::size_t vertices = 0, faces = 0, materials = 0;
};

/// Escribe `basePath`.obj y `basePath`.mtl. Devuelve false en caso de error.
bool exportObj(const VoxelGrid& grid, const std::string& objPath, MeshStats* stats = nullptr,
               std::string* error = nullptr);
/// Version en memoria (para pruebas): devuelve el texto OBJ y MTL.
MeshStats buildObj(const VoxelGrid& grid, const std::string& mtlFileName, std::string& obj, std::string& mtl);

}  // namespace xeldi::io
