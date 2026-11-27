#include "engine2d/BrushTools.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace xeldi {

std::vector<Point> bresenhamLine(Point a, Point b) {
    std::vector<Point> pts;
    int dx = std::abs(b.x - a.x), sx = a.x < b.x ? 1 : -1;
    int dy = -std::abs(b.y - a.y), sy = a.y < b.y ? 1 : -1;
    int err = dx + dy;
    Point p = a;
    for (;;) {
        pts.push_back(p);
        if (p == b) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; p.x += sx; }
        if (e2 <= dx) { err += dx; p.y += sy; }
    }
    return pts;
}

std::vector<Point> pixelPerfect(const std::vector<Point>& path) {
    if (path.size() < 3) return path;
    std::vector<Point> out;
    out.push_back(path[0]);
    for (std::size_t i = 1; i + 1 < path.size(); ++i) {
        const Point& prev = out.back();
        const Point& cur = path[i];
        const Point& next = path[i + 1];
        bool lCorner = (prev.x == cur.x || prev.y == cur.y) && (next.x == cur.x || next.y == cur.y) &&
                       prev.x != next.x && prev.y != next.y;
        if (!lCorner) out.push_back(cur);
    }
    out.push_back(path.back());
    return out;
}

Rect stampBrush(Image& img, Point p, Rgba color, const BrushOptions& o) {
    int s = std::max(1, o.size);
    int x0 = p.x - (s - 1) / 2, y0 = p.y - (s - 1) / 2;
    float c = (s - 1) / 2.0f, r2 = (s / 2.0f) * (s / 2.0f);
    for (int y = 0; y < s; ++y)
        for (int x = 0; x < s; ++x) {
            if (o.round && s >= 3) {
                float dx = x - c, dy = y - c;
                if (dx * dx + dy * dy > r2 + 0.25f) continue;
            }
            img.setPixel(x0 + x, y0 + y, color);
        }
    return Rect{x0, y0, s, s}.intersected(img.rect());
}

Rect drawLine(Image& img, Point a, Point b, Rgba color, const BrushOptions& o) {
    Rect r;
    for (const Point& p : bresenhamLine(a, b)) r = r.united(stampBrush(img, p, color, o));
    return r;
}

Rect drawRectangle(Image& img, Point a, Point b, Rgba color, bool filled, const BrushOptions& o) {
    int x0 = std::min(a.x, b.x), x1 = std::max(a.x, b.x);
    int y0 = std::min(a.y, b.y), y1 = std::max(a.y, b.y);
    if (filled) {
        for (int y = y0; y <= y1; ++y)
            for (int x = x0; x <= x1; ++x) img.setPixel(x, y, color);
        return Rect{x0, y0, x1 - x0 + 1, y1 - y0 + 1}.intersected(img.rect());
    }
    Rect r = drawLine(img, {x0, y0}, {x1, y0}, color, o);
    r = r.united(drawLine(img, {x1, y0}, {x1, y1}, color, o));
    r = r.united(drawLine(img, {x1, y1}, {x0, y1}, color, o));
    r = r.united(drawLine(img, {x0, y1}, {x0, y0}, color, o));
    return r;
}

Rect drawEllipse(Image& img, Point a, Point b, Rgba color, bool filled) {
    int x0 = std::min(a.x, b.x), x1 = std::max(a.x, b.x);
    int y0 = std::min(a.y, b.y), y1 = std::max(a.y, b.y);
    float cx = (x0 + x1) / 2.0f, cy = (y0 + y1) / 2.0f;
    float rx = (x1 - x0) / 2.0f + 0.5f, ry = (y1 - y0) / 2.0f + 0.5f;
    auto inside = [&](int x, int y) {
        float dx = (x - cx) / rx, dy = (y - cy) / ry;
        return dx * dx + dy * dy <= 1.0f;
    };
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) {
            if (!inside(x, y)) continue;
            bool edge = !inside(x - 1, y) || !inside(x + 1, y) || !inside(x, y - 1) || !inside(x, y + 1);
            if (filled || edge) img.setPixel(x, y, color);
        }
    return Rect{x0, y0, x1 - x0 + 1, y1 - y0 + 1}.intersected(img.rect());
}

Rect floodFill(Image& img, Point p, Rgba color, bool contiguous) {
    if (!img.contains(p.x, p.y)) return {};
    Rgba target = img.pixel(p.x, p.y);
    if (target == color) return {};
    int minX = p.x, maxX = p.x, minY = p.y, maxY = p.y;
    auto grow = [&](int x, int y) {
        minX = std::min(minX, x); maxX = std::max(maxX, x);
        minY = std::min(minY, y); maxY = std::max(maxY, y);
    };
    if (!contiguous) {
        for (int y = 0; y < img.height(); ++y)
            for (int x = 0; x < img.width(); ++x)
                if (img.pixel(x, y) == target) { img.setPixel(x, y, color); grow(x, y); }
        return {minX, minY, maxX - minX + 1, maxY - minY + 1};
    }
    std::vector<Point> stack{p};
    while (!stack.empty()) {
        Point q = stack.back();
        stack.pop_back();
        if (!img.contains(q.x, q.y) || img.pixel(q.x, q.y) != target) continue;
        // Recorrido por lineas horizontales.
        int l = q.x, r = q.x;
        while (img.contains(l - 1, q.y) && img.pixel(l - 1, q.y) == target) --l;
        while (img.contains(r + 1, q.y) && img.pixel(r + 1, q.y) == target) ++r;
        for (int x = l; x <= r; ++x) {
            img.setPixel(x, q.y, color);
            if (img.contains(x, q.y - 1) && img.pixel(x, q.y - 1) == target) stack.push_back({x, q.y - 1});
            if (img.contains(x, q.y + 1) && img.pixel(x, q.y + 1) == target) stack.push_back({x, q.y + 1});
        }
        grow(l, q.y);
        grow(r, q.y);
    }
    return {minX, minY, maxX - minX + 1, maxY - minY + 1};
}

}  // namespace xeldi
