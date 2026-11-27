#pragma once
#include <memory>
#include <string>
#include <vector>

#include "core/Image.hpp"

namespace xeldi {

enum class LayerType { Raster2D, Spatial3D };
enum class BlendMode { Normal, Multiply, Screen, Add, Darken, Lighten, Overlay };

const char* toString(LayerType t);
LayerType layerTypeFromString(const std::string& s);
const char* toString(BlendMode b);
BlendMode blendModeFromString(const std::string& s);

/// Capa. Para capas raster guarda una "cel" (imagen) por frame; nullptr = cel vacia.
/// Las capas Spatial3D representan la escena 3D dentro de la pila de capas
/// (su contenido vive en Document::scene).
struct Layer {
    std::string id;
    std::string name;
    LayerType type = LayerType::Raster2D;
    bool visible = true;
    bool locked = false;
    float opacity = 1.0f;
    BlendMode blend = BlendMode::Normal;
    std::string parentId;  ///< reservado para grupos
    std::vector<std::shared_ptr<Image>> cels;
};

}  // namespace xeldi
