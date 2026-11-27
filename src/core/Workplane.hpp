#pragma once
// Plano de trabajo (estilo Mental Canvas, pero en pixeles).
// Cada plano tiene su propio bitmap RGBA alineado a la rejilla:
//   punto(u, v) = origin + right * u + up * v      (u, v en unidades = pixeles)
// La fila 0 de la imagen es la de arriba (v = heightPx - 1).
#include <optional>
#include <string>

#include "core/Image.hpp"
#include "core/Math.hpp"

namespace xeldi {

enum class PlaneAxis { Front, Side, Top };

struct PlanePixel {
    int px = 0, py = 0;  ///< coordenadas en la imagen del plano
    Vec3 world;          ///< punto de impacto en el mundo
};

struct Workplane {
    std::string id;
    std::string name;
    Vec3 origin{0, 0, 0};
    Vec3 normal{0, 0, 1};
    Vec3 right{1, 0, 0};
    Vec3 up{0, 1, 0};
    int widthPx = 32;
    int heightPx = 32;
    bool snapToGrid = true;
    bool visible = true;
    Image pixels;  ///< widthPx x heightPx

    /// Plano alineado a ejes, centrado en X/Z, desplazado `offset` a lo largo de su normal.
    static Workplane axisAligned(PlaneAxis axis, int offset, int size, std::string id, std::string name);

    void ensurePixels();
    /// Centro en el mundo del pixel (px, py) de la imagen.
    Vec3 pixelCenter(int px, int py) const;
    /// Esquina del plano en coordenadas (u, v) del plano.
    Vec3 pointAt(float u, float v) const { return origin + right * u + up * v; }
    /// Convierte un punto del mundo (sobre el plano) en pixel. nullopt si cae fuera.
    std::optional<PlanePixel> worldToPixel(Vec3 p) const;
    /// Mueve el plano `steps` unidades a lo largo de la normal.
    void moveAlongNormal(int steps) { origin += normal * float(steps); }
};

}  // namespace xeldi
