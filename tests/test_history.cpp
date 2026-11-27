#include "Test.hpp"
#include "core/Commands.hpp"
#include "engine2d/BrushTools.hpp"

using namespace xeldi;

TEST("un trazo guarda solo el rectangulo modificado") {
    Document d = Document::createBlank(256, 256);
    History h;
    PixelStroke s(d, 0, 0);
    s.markDirty(drawLine(s.target(), {10, 10}, {20, 12}, 0xFF00FFFFu));
    auto cmd = s.finish("Linea");
    CHECK(cmd != nullptr);
    auto* patch = dynamic_cast<PixelPatchCommand*>(cmd.get());
    CHECK(patch && patch->rect() == (Rect{10, 10, 11, 3}));
    CHECK(cmd->memoryBytes() < 1024);  // muy lejos de 256*256*4
    h.push(std::move(cmd));
    CHECK(d.cel(0, 0)->pixel(10, 10) == 0xFF00FFFFu);
    h.undo(d);
    CHECK(d.cel(0, 0) == nullptr);  // la cel estaba vacia: vuelve a vacia
    h.redo(d);
    CHECK(d.cel(0, 0)->pixel(20, 12) == 0xFF00FFFFu);
}

TEST("trazo sin cambios no genera comando") {
    Document d = Document::createBlank(8, 8);
    PixelStroke s(d, 0, 0);
    s.markDirty({0, 0, 8, 8});
    CHECK(s.finish("nada") == nullptr);
    CHECK(d.cel(0, 0) == nullptr);
}

TEST("estado limpio y rehacer descartado") {
    Document d = Document::createBlank(8, 8);
    History h;
    auto paint = [&](int x) {
        PixelStroke s(d, 0, 0);
        s.target().setPixel(x, 0, 0xFFu | 0xAA000000u);
        s.markDirty({x, 0, 1, 1});
        h.push(s.finish("p"));
    };
    paint(0);
    h.markClean();
    CHECK(h.isClean());
    paint(1);
    CHECK(!h.isClean());
    h.undo(d);
    CHECK(h.isClean());
    h.undo(d);
    paint(2);
    CHECK(!h.canRedo());
    CHECK(!h.isClean());
}

TEST("comandos estructurales comparten cels (baratos)") {
    Document d = Document::createBlank(8, 8);
    d.ensureCel(0, 0).setPixel(0, 0, 0xFFFFFFFFu);
    auto before = StructureState::capture(d);
    d.insertRasterLayer(1, "otra");
    auto after = StructureState::capture(d);
    CHECK(before.layers[0].cels[0].get() == d.layers[0].cels[0].get());
    StructureCommand c("capa", before, after, ChangeLayers);
    c.undo(d);
    CHECK_EQ(int(d.layers.size()), 1);
    c.redo(d);
    CHECK_EQ(int(d.layers.size()), 2);
}

TEST("limite de memoria del historial") {
    Document d = Document::createBlank(64, 64);
    History h;
    h.maxBytes = 4096;
    for (int i = 0; i < 50; ++i) {
        PixelStroke s(d, 0, 0);
        s.markDirty(drawRectangle(s.target(), {0, 0}, {20, 20}, 0xFF000000u | Rgba(i + 1), true));
        h.push(s.finish("r"));
    }
    CHECK(h.undoCount() < 50);
    CHECK(h.undoCount() >= 1);
}

TEST_MAIN()
