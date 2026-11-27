#pragma once
// Onion skinning: frames anteriores (tinte rosa) y siguientes (tinte azul)
// con opacidad decreciente.
#include <vector>

#include "core/Document.hpp"

namespace xeldi {

struct OnionGhost {
    int frame = 0;
    int distance = 1;  ///< 1 = vecino inmediato
    bool past = true;
    float opacity = 0;
    Rgba tint = 0;
};

/// Calcula que frames se muestran como fantasmas. Si `loopTag` no es nulo y el
/// frame actual esta dentro de el, los fantasmas dan la vuelta dentro del tag.
std::vector<OnionGhost> onionGhosts(const Document& doc, int frame, const OnionSkinSettings& s,
                                    const AnimationTag* loopTag = nullptr);

/// Tine un pixel para fantasma.
Rgba tintGhostPixel(Rgba c, Rgba tint, float opacity);

/// Dibuja los fantasmas sobre `out` (tamano del documento). `currentLayer` se usa
/// si `onlyCurrentLayer` esta activo.
void renderOnion(const Document& doc, int frame, const OnionSkinSettings& s, int currentLayer, Image& out,
                 const AnimationTag* loopTag = nullptr);

}  // namespace xeldi
