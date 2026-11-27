#pragma once
// Composicion de capas 2D (modos de fusion, opacidad).
#include "core/Document.hpp"

namespace xeldi {

struct ComposeOptions {
    bool includeHidden = false;
    int onlyLayer = -1;          ///< >=0: solo esa capa
    Rgba background = kTransparent;
};

/// Mezcla `src` sobre `dst` con el modo y la opacidad dados (formula W3C separable).
Rgba blendPixel(Rgba dst, Rgba src, BlendMode mode, float opacity = 1.0f);

/// Compone una capa sobre `out` (mismo tamano).
void composeImage(Image& out, const Image& src, BlendMode mode, float opacity);

/// Imagen final de un frame (capas raster visibles, de abajo hacia arriba).
Image composeFrame(const Document& doc, int frame, const ComposeOptions& opt = {});

}  // namespace xeldi
