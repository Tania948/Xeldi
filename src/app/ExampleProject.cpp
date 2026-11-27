#include "app/ExampleProject.hpp"

#include <QImage>

#include "app/QtBridge.hpp"
#include "engine2d/BrushTools.hpp"
#include "engine3d/Voxelize.hpp"

namespace xeldi::ui {

Document makeBunnyExample() {
    Document d = Document::createBlank(32, 32);
    d.name = "Conejito Xeldi";
    d.layers[0].name = "Sombra";
    Layer& body = d.insertRasterLayer(1, "Conejito");
    (void)body;
    const int durations[] = {220, 180, 220, 120};
    for (int i = 1; i < 4; ++i) d.insertFrame(i);
    for (int i = 0; i < 4; ++i) {
        d.timeline.frames[i].durationMs = durations[i];
        QImage q(QStringLiteral(":/mascot/bunny_idle_%1.png").arg(i));
        if (!q.isNull()) d.layers[1].cels[i] = std::make_shared<Image>(fromQImage(q));
        Image& shadow = d.ensureCel(0, i);
        drawEllipse(shadow, {7, 29}, {24, 31}, 0xC9A8E8AAu, true);
    }
    d.layers[0].opacity = 0.7f;
    AnimationTag idle;
    idle.name = "idle";
    idle.from = 0;
    idle.to = 3;
    idle.autoPreview = true;
    d.timeline.tags.push_back(idle);

    // Version 3D: el sprite en el plano frontal, extruido 4 voxeles.
    auto& sc = d.scene;
    sc.workplanes.clear();
    Workplane front = Workplane::axisAligned(PlaneAxis::Front, 0, 32, "plane_front", "Plano frontal");
    if (d.layers[1].cels[0]) front.pixels = *d.layers[1].cels[0];
    extrudeWorkplane(front, 4, [&](IVec3 p, Voxel v) { sc.voxels.set(p, v); });
    front.moveAlongNormal(-1);
    front.pixels = Image(32, 32);
    sc.workplanes.push_back(Workplane::axisAligned(PlaneAxis::Top, 0, 32, "plane_floor", "Piso"));
    sc.workplanes.push_back(front);
    sc.activeWorkplane = 0;
    sc.camera.target = {0, 14, 2};
    sc.camera.distance = 70;
    sc.camera.yawDeg = 30;
    sc.camera.pitchDeg = 20;
    return d;
}

}  // namespace xeldi::ui
