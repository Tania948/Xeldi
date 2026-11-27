#include "Test.hpp"
#include "editor/Editor.hpp"

using namespace xeldi;

TEST("lapiz + deshacer/rehacer via acciones con nombre") {
    Editor ed;
    int notified = 0;
    ed.subscribe([&](unsigned) { ++notified; });
    ed.setPrimaryColor(0xE86A92FFu);
    ed.beginStroke2D({2, 2});
    ed.moveStroke2D({6, 2});
    ed.endStroke2D({6, 2});
    CHECK(ed.doc().cel(0, 0) && ed.doc().cel(0, 0)->pixel(4, 2) == 0xE86A92FFu);
    CHECK(ed.actions().isEnabled("edit.undo"));
    CHECK(ed.actions().trigger("edit.undo"));
    CHECK(ed.doc().cel(0, 0) == nullptr);
    CHECK(ed.actions().trigger("edit.redo"));
    CHECK(ed.doc().cel(0, 0)->pixel(6, 2) == 0xE86A92FFu);
    CHECK(notified > 0);
    CHECK(!ed.actions().trigger("no.existe"));
}

TEST("vista previa de linea no deja rastros") {
    Editor ed;
    ed.setTool2D(Tool2D::Line);
    ed.beginStroke2D({0, 0});
    ed.moveStroke2D({10, 0});
    ed.moveStroke2D({0, 10});
    ed.endStroke2D({0, 10});
    const Image* c = ed.doc().cel(0, 0);
    CHECK(c && c->pixel(0, 5) != 0);
    CHECK(c && c->pixel(5, 0) == 0);
}

TEST("frames, capas y tags deshacibles") {
    Editor ed;
    ed.actions().trigger("frame.add");
    ed.actions().trigger("frame.add");
    CHECK_EQ(ed.doc().frameCount(), 3);
    CHECK_EQ(ed.currentFrame(), 2);
    ed.addTagFromSelection("idle", 0, 2);
    CHECK(ed.doc().timeline.previewTag() != nullptr);
    ed.actions().trigger("layer.add");
    CHECK_EQ(int(ed.doc().layers.size()), 2);
    ed.undo();
    ed.undo();
    CHECK_EQ(int(ed.doc().layers.size()), 1);
    CHECK(ed.doc().timeline.tags.empty());
    CHECK(ed.doc().validate());
}

TEST("reproduccion en el lienzo avanza frames") {
    Editor ed;
    ed.addFrame();
    ed.addFrame();
    ed.setCurrentFrame(0);
    ed.actions().trigger("anim.play");
    CHECK(ed.isPlaying());
    ed.tick(100);
    CHECK_EQ(ed.currentFrame(), 1);
    ed.actions().trigger("anim.play");
    CHECK(!ed.isPlaying());
}

TEST("colocar voxeles desde la vista 3D y deshacer") {
    Editor ed;
    auto& cam = ed.camera();
    applyViewPreset(cam, ViewPreset::Front);
    cam.target = {0.5f, 0.5f, 0};
    ed.setTool3D(Tool3D::PlaceVoxel);
    ed.beginStroke3D({100, 100}, 200, 200);  // centro -> plano frontal z=0
    ed.endStroke3D();
    CHECK_EQ(int(ed.doc().scene.voxels.voxelCount()), 1);
    CHECK(ed.doc().scene.voxels.has({0, 0, 0}));
    ed.undo();
    CHECK(ed.doc().scene.voxels.empty());
}

TEST("dibujar en plano, extruir y proyectar a sprite") {
    Editor ed;
    auto& cam = ed.camera();
    applyViewPreset(cam, ViewPreset::Front);
    cam.target = {0.5f, 0.5f, 0};
    ed.setTool3D(Tool3D::DrawOnPlane);
    ed.setPrimaryColor(0xA775C9FFu);
    ed.beginStroke3D({100, 100}, 200, 200);
    ed.endStroke3D();
    auto* pl = ed.doc().scene.activePlane();
    CHECK(pl && !pl->pixels.isFullyTransparent());
    ed.setExtrudeDepth(2);
    CHECK(ed.actions().trigger("plane.extrude"));
    CHECK_EQ(int(ed.doc().scene.voxels.voxelCount()), 2);
    ed.projectToCel(OrthoView::Front, true);
    CHECK_EQ(int(ed.doc().layers.size()), 2);
    const Image* c = ed.doc().cel(ed.currentLayer(), 0);
    CHECK(c && c->pixel(32, 63) == 0xA775C9FFu);
}

TEST("generar desde frames") {
    Editor ed;
    Image& f = ed.doc().ensureCel(0, 0);
    for (int y = 50; y < 64; ++y)
        for (int x = 28; x < 36; ++x) f.setPixel(x, y, 0xF29BB8FFu);
    CHECK(ed.generateFromFrames(0, -1, -1, true));
    // Sin vista lateral ni superior: la profundidad es todo el lienzo.
    CHECK_EQ(int(ed.doc().scene.voxels.voxelCount()), 8 * 14 * 64);
    ed.undo();
    CHECK(ed.doc().scene.voxels.empty());
}

TEST_MAIN()
