#include "Test.hpp"
#include "core/Document.hpp"
#include "engine2d/BrushTools.hpp"
#include "engine2d/Compositor.hpp"
#include "engine2d/OnionSkin.hpp"

using namespace xeldi;

TEST("color hex ida y vuelta") {
    CHECK_EQ(toHex(0xF29BB8FFu), std::string("#F29BB8FF"));
    CHECK(*fromHex("#F29BB8") == 0xF29BB8FFu);
    CHECK(*fromHex("fab") == 0xFFAABBFFu);
    CHECK(!fromHex("#zz0000"));
}

TEST("linea de Bresenham sin huecos") {
    auto pts = bresenhamLine({0, 0}, {5, 2});
    CHECK_EQ(int(pts.size()), 6);
    CHECK(pts.front() == (Point{0, 0}));
    CHECK(pts.back() == (Point{5, 2}));
    for (std::size_t i = 1; i < pts.size(); ++i) {
        CHECK(std::abs(pts[i].x - pts[i - 1].x) <= 1);
        CHECK(std::abs(pts[i].y - pts[i - 1].y) <= 1);
    }
}

TEST("pincel escribe color exacto, sin antialiasing") {
    Image img(8, 8);
    Rect r = stampBrush(img, {3, 3}, 0x123456FFu, {3, false});
    CHECK(r == (Rect{2, 2, 3, 3}));
    int painted = 0;
    for (auto c : img.pixels()) {
        CHECK(c == 0 || c == 0x123456FFu);
        painted += c != 0;
    }
    CHECK_EQ(painted, 9);
}

TEST("pixel perfect elimina esquinas en L") {
    std::vector<Point> path{{0, 0}, {1, 0}, {1, 1}, {2, 1}, {2, 2}};
    auto out = pixelPerfect(path);
    CHECK(out.size() < path.size());
    CHECK(out.front() == (Point{0, 0}));
    CHECK(out.back() == (Point{2, 2}));
}

TEST("relleno por inundacion respeta bordes") {
    Image img(5, 5);
    drawRectangle(img, {0, 0}, {4, 4}, 0xFF0000FFu, false);
    Rect r = floodFill(img, {2, 2}, 0x00FF00FFu);
    CHECK(r == (Rect{1, 1, 3, 3}));
    CHECK(img.pixel(0, 0) == 0xFF0000FFu);
    CHECK(img.pixel(2, 2) == 0x00FF00FFu);
}

TEST("composicion normal y multiply") {
    CHECK(blendPixel(0xFFFFFFFFu, 0xFF000080u, BlendMode::Normal) != 0xFFFFFFFFu);
    Rgba m = blendPixel(0x808080FFu, 0xFF0000FFu, BlendMode::Multiply);
    CHECK_EQ(int(red(m)), 128);
    CHECK_EQ(int(green(m)), 0);
    // Pixel opaco sobre transparente: queda igual.
    CHECK(blendPixel(0, 0x11223344u, BlendMode::Normal, 1.0f) == 0x11223344u);
}

TEST("composeFrame ignora capas ocultas") {
    Document d = Document::createBlank(4, 4);
    d.ensureCel(0, 0).setPixel(1, 1, 0xFF0000FFu);
    d.insertRasterLayer(1, "arriba");
    d.ensureCel(1, 0).setPixel(1, 1, 0x0000FFFFu);
    CHECK(composeFrame(d, 0).pixel(1, 1) == 0x0000FFFFu);
    d.layers[1].visible = false;
    CHECK(composeFrame(d, 0).pixel(1, 1) == 0xFF0000FFu);
}

TEST("onion skin: fantasmas, tintes y bucle dentro del tag") {
    Document d = Document::createBlank(4, 4);
    for (int i = 0; i < 5; ++i) d.insertFrame(d.frameCount());
    OnionSkinSettings s;
    s.prevFrames = 2;
    s.nextFrames = 1;
    auto g = onionGhosts(d, 0, s);
    CHECK_EQ(int(g.size()), 1);  // en el frame 0 no hay anteriores sin tag
    CHECK(!g[0].past);
    AnimationTag tag{"idle", 0, 3};
    auto gl = onionGhosts(d, 0, s, &tag);
    CHECK_EQ(int(gl.size()), 3);
    bool hasWrap = false;
    for (auto& x : gl) hasWrap |= (x.past && x.frame == 3);
    CHECK(hasWrap);
    // Opacidad decreciente con la distancia
    auto g2 = onionGhosts(d, 3, s);
    float d1 = -1, d2 = -1;
    for (auto& x : g2)
        if (x.past) (x.distance == 1 ? d1 : d2) = x.opacity;
    CHECK(d1 > 0 && d2 > 0 && d2 < d1);
    s.enabled = false;
    CHECK(onionGhosts(d, 2, s).empty());
}

TEST_MAIN()
