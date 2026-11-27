#include "Test.hpp"
#include "core/Document.hpp"
#include "engine3d/CameraMath.hpp"
#include "engine3d/Picking.hpp"
#include "engine3d/SoftwareRenderer.hpp"
#include "engine3d/ViewProjection.hpp"
#include "engine3d/Voxelize.hpp"

using namespace xeldi;

TEST("raycast DDA encuentra el voxel y la cara") {
    VoxelGrid g;
    g.set({0, 0, 0}, {0xFFFFFFFFu});
    Ray r{{0.5f, 0.5f, 10.0f}, {0, 0, -1}};
    auto hit = raycastVoxels(g, r);
    CHECK(hit.has_value());
    CHECK(hit->cell == (IVec3{0, 0, 0}));
    CHECK(hit->normal == (IVec3{0, 0, 1}));
    CHECK(hit->adjacent() == (IVec3{0, 0, 1}));
    Ray miss{{5.5f, 0.5f, 10.0f}, {0, 0, -1}};
    CHECK(!raycastVoxels(g, miss));
}

TEST("rayo de pantalla al centro apunta al objetivo") {
    CameraState cam;
    cam.target = {0, 0, 0};
    Ray r = screenRay(cam, 200, 100, 100, 50);
    auto t = intersectPlane({0, 0, 0}, normalize(cam.position() - cam.target), r);
    CHECK(t.has_value());
    Vec3 p = r.at(*t);
    CHECK(length(p) < 0.01f);
    cam.ortho = true;
    Ray ro = screenRay(cam, 200, 100, 100, 50);
    auto t2 = intersectPlane({0, 0, 0}, normalize(cam.position() - cam.target), ro);
    CHECK(t2 && length(ro.at(*t2)) < 0.01f);
}

TEST("plano de trabajo: pixel <-> mundo") {
    Workplane w = Workplane::axisAligned(PlaneAxis::Front, 2, 16, "p", "p");
    Vec3 c = w.pixelCenter(0, 15);  // abajo a la izquierda
    CHECK(std::fabs(c.x - (-7.5f)) < 1e-4f && std::fabs(c.y - 0.5f) < 1e-4f && std::fabs(c.z - 2) < 1e-4f);
    auto px = w.worldToPixel(c);
    CHECK(px && px->px == 0 && px->py == 15);
    Ray r{{-7.5f + 3, 4.5f, 20}, {0, 0, -1}};
    auto hit = pickWorkplane(w, r);
    CHECK(hit && hit->px == 3 && hit->py == 11);
}

TEST("extruir plano frontal crea voxeles hacia +Z") {
    Workplane w = Workplane::axisAligned(PlaneAxis::Front, 0, 8, "p", "p");
    w.pixels.setPixel(4, 7, 0xE86A92FFu);  // x = 0, y = 0
    VoxelGrid g;
    extrudeWorkplane(w, 3, [&](IVec3 p, Voxel v) { g.set(p, v); });
    CHECK_EQ(int(g.voxelCount()), 3);
    CHECK(g.has({0, 0, 0}) && g.has({0, 0, 1}) && g.has({0, 0, 2}));
    VoxelGrid back;
    extrudeWorkplane(w, -2, [&](IVec3 p, Voxel v) { back.set(p, v); });
    CHECK(back.has({0, 0, -1}) && back.has({0, 0, -2}));
}

TEST("bresenham 3D conecta celdas sin huecos") {
    auto pts = bresenham3D({0, 0, 0}, {5, 3, -2});
    CHECK(pts.front() == (IVec3{0, 0, 0}));
    CHECK(pts.back() == (IVec3{5, 3, -2}));
    for (std::size_t i = 1; i < pts.size(); ++i) {
        IVec3 d = pts[i] - pts[i - 1];
        CHECK(std::abs(d.x) <= 1 && std::abs(d.y) <= 1 && std::abs(d.z) <= 1);
    }
}

TEST("proyeccion ortografica exacta") {
    VoxelGrid g;
    g.set({0, 0, 0}, {0xAA0000FFu});
    g.set({0, 0, 3}, {0x00AA00FFu});  // mas cerca de la camara frontal
    Image f = projectVoxels(g, OrthoView::Front, 16, 16);
    CHECK(f.pixel(8, 15) == 0x00AA00FFu);
    Image b = projectVoxels(g, OrthoView::Back, 16, 16);
    CHECK(b.pixel(7, 15) == 0xAA0000FFu);
    Image t = projectVoxels(g, OrthoView::Top, 16, 16);
    CHECK(t.pixel(8, 8) == 0xAA0000FFu && t.pixel(8, 11) == 0x00AA00FFu);
}

TEST("generar desde vistas reproduce las siluetas") {
    const int W = 16, H = 16;
    Image front(W, H), side(W, H), top(W, H);
    // Cuerpo 4x6 al frente, 2 de profundidad en el lado, y la planta coincide.
    for (int y = 10; y < 16; ++y)
        for (int x = 6; x < 10; ++x) front.setPixel(x, y, 0xF29BB8FFu);
    for (int y = 10; y < 16; ++y)
        for (int x = 7; x < 9; ++x) side.setPixel(x, y, 0xB8C4FFFFu);
    for (int y = 7; y < 9; ++y)
        for (int x = 6; x < 10; ++x) top.setPixel(x, y, 0xC9A8E8FFu);
    VoxelGrid g = generateFromViews({&front, &side, &top});
    CHECK_EQ(int(g.voxelCount()), 4 * 6 * 2);
    auto alphaMask = [](const Image& a) {
        std::vector<bool> m;
        for (auto c : a.pixels()) m.push_back(alpha(c) != 0);
        return m;
    };
    CHECK(alphaMask(projectVoxels(g, OrthoView::Front, W, H)) == alphaMask(front));
    CHECK(alphaMask(projectVoxels(g, OrthoView::Right, W, H)) == alphaMask(side));
    CHECK(alphaMask(projectVoxels(g, OrthoView::Top, W, H)) == alphaMask(top));
    // Colores: el frente conserva el color de la vista frontal.
    CHECK(projectVoxels(g, OrthoView::Front, W, H).pixel(7, 12) == 0xF29BB8FFu);
    CHECK(projectVoxels(g, OrthoView::Right, W, H).pixel(8, 12) == 0xB8C4FFFFu);
}

TEST("renderizador por software dibuja voxeles") {
    Scene3D s;
    s.voxels.set({0, 0, 0}, {0xE86A92FFu});
    s.camera.target = {0.5f, 0.5f, 0.5f};
    s.camera.distance = 6;
    SoftwareRenderer r;
    RenderOptions o;
    o.showGrid = false;
    o.shading = false;
    const Image& img = r.render(s, s.camera, 64, 64, o);
    CHECK(img.pixel(32, 32) == 0xE86A92FFu);
    CHECK(img.pixel(1, 1) == o.background);
}

TEST_MAIN()
