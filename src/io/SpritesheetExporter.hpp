#pragma once
// Hoja de sprites (PNG) + metadatos JSON con rectangulos, duraciones y tags.
#include <string>

#include "core/Document.hpp"

namespace xeldi::io {

struct SpritesheetOptions {
    int from = 0;
    int to = -1;       ///< -1 = ultimo frame
    int columns = 0;   ///< 0 = automatico (cuadrado aproximado)
    int padding = 0;
    int scale = 1;     ///< escalado entero (vecino mas cercano)
};

struct Spritesheet {
    Image image;
    std::string json;
};

Spritesheet buildSpritesheet(const Document& doc, const SpritesheetOptions& opt = {});
bool exportSpritesheet(const Document& doc, const std::string& pngPath, const SpritesheetOptions& opt = {},
                       std::string* error = nullptr);

}  // namespace xeldi::io
