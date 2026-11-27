#pragma once
// Herramientas de pixel puro: sin antialiasing, escriben el color exacto.
#include <vector>

#include "core/Image.hpp"

namespace xeldi {

struct Point {
    int x = 0, y = 0;
    bool operator==(const Point&) const = default;
};

struct BrushOptions {
    int size = 1;        ///< lado del pincel en pixeles
    bool round = false;  ///< pincel circular (size >= 3)
};

/// Puntos de una linea de Bresenham (incluye extremos).
std::vector<Point> bresenhamLine(Point a, Point b);

/// Filtro "pixel perfect": elimina las esquinas en L de un trazo a mano alzada.
std::vector<Point> pixelPerfect(const std::vector<Point>& path);

/// Todas devuelven el rectangulo afectado (para el historial).
Rect stampBrush(Image& img, Point p, Rgba color, const BrushOptions& o = {});
Rect drawLine(Image& img, Point a, Point b, Rgba color, const BrushOptions& o = {});
Rect drawRectangle(Image& img, Point a, Point b, Rgba color, bool filled, const BrushOptions& o = {});
Rect drawEllipse(Image& img, Point a, Point b, Rgba color, bool filled);
/// Relleno por inundacion (4-conexo). contiguous=false reemplaza el color en todo el lienzo.
Rect floodFill(Image& img, Point p, Rgba color, bool contiguous = true);

}  // namespace xeldi
