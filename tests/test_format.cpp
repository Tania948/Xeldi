#include "Test.hpp"

#include <filesystem>
#include <fstream>

#include "core/Document.hpp"
#include "io/Binary.hpp"
#include "io/MeshExporter.hpp"
#include "io/Png.hpp"
#include "io/SpritesheetExporter.hpp"
#include "io/XeldiFormat.hpp"

using namespace xeldi;

static Document sample() {
    Document d = Document::createBlank(32, 24);
    d.name = "ConejitoIdle";
    d.insertFrame(1);
    d.insertFrame(2);
    d.timeline.frames[2].durationMs = 160;
    d.timeline.tags.push_back({"idle", 0, 2, LoopMode::PingPong, 10, true});
    d.ensureCel(0, 0).setPixel(3, 4, 0xF29BB8FFu);
    d.ensureCel(0, 2).setPixel(31, 23, 0x5A3D52FFu);
    Layer& l = d.insertRasterLayer(1, "Ojos");
    l.opacity = 0.5f;
    l.blend = BlendMode::Multiply;
    l.visible = false;
    d.onion.prevFrames = 3;
    d.onion.prevTint = 0x123456FFu;
    d.scene.voxels.set({-20, 3, 5}, {0xA775C9FFu, 7});
    for (int z = 0; z < 16; ++z)
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x) d.scene.voxels.set({x, y, z}, {0xB8C4FFFFu});  // chunk denso
    d.scene.workplanes.push_back(Workplane::axisAligned(PlaneAxis::Top, 4, 8, "plane_top", "Arriba"));
    d.scene.workplanes.back().pixels.setPixel(1, 1, 0xFFD8BEFFu);
    d.scene.activeWorkplane = 1;
    d.scene.camera.yawDeg = 60;
    d.scene.camera.ortho = true;
    return d;
}

TEST("PNG codificar/decodificar") {
    Image img(7, 5);
    img.setPixel(0, 0, 0x11223344u);
    img.setPixel(6, 4, 0xF29BB8FFu);
    auto bytes = io::encodePng(img);
    auto back = io::decodePng(bytes.data(), bytes.size());
    CHECK(back.has_value());
    CHECK(back && *back == img);
}

TEST(".xeldi ida y vuelta conserva todo") {
    Document d = sample();
    auto bytes = io::serializeDocument(d);
    CHECK(bytes.size() > 32);
    CHECK(std::string(reinterpret_cast<const char*>(bytes.data()), 4) == "XELD");
    auto r = io::deserializeDocument(bytes.data(), bytes.size());
    CHECK(r.ok);
    if (!r.ok) { std::printf("  error: %s\n", r.error.c_str()); return; }
    const Document& e = r.document;
    CHECK(e.name == d.name);
    CHECK_EQ(e.width, 32);
    CHECK_EQ(e.frameCount(), 3);
    CHECK_EQ(e.timeline.frames[2].durationMs, 160);
    CHECK(e.timeline.tags.size() == 1 && e.timeline.tags[0].loop == LoopMode::PingPong);
    CHECK(e.timeline.tags[0].autoPreview && e.timeline.tags[0].fpsOverride == 10);
    CHECK(e.layers.size() == 2 && e.layers[1].name == "Ojos" && e.layers[1].blend == BlendMode::Multiply);
    CHECK(!e.layers[1].visible && e.layers[1].opacity == 0.5f);
    CHECK(e.layers[0].id == d.layers[0].id);
    CHECK(e.cel(0, 0) && e.cel(0, 0)->pixel(3, 4) == 0xF29BB8FFu);
    CHECK(e.cel(0, 1) == nullptr);  // celdas vacias se omiten
    CHECK(e.cel(0, 2) && e.cel(0, 2)->pixel(31, 23) == 0x5A3D52FFu);
    CHECK(e.onion.prevFrames == 3 && e.onion.prevTint == 0x123456FFu);
    CHECK(e.scene.voxels == d.scene.voxels);
    CHECK(e.scene.voxels.get({-20, 3, 5}).flags == 7);
    CHECK(e.scene.workplanes.size() == 2 && e.scene.workplanes[1].pixels.pixel(1, 1) == 0xFFD8BEFFu);
    CHECK(e.scene.workplanes[1].normal == (Vec3{0, 1, 0}));
    CHECK_EQ(e.scene.activeWorkplane, 1);
    CHECK(e.scene.camera.ortho && std::fabs(e.scene.camera.yawDeg - 60) < 1e-3f);
    CHECK(e.palette.colors == d.palette.colors);
}

TEST("archivo corrupto se rechaza con mensaje") {
    auto bytes = io::serializeDocument(sample());
    auto bad = bytes;
    bad[bad.size() / 2] ^= 0x5A;
    auto r = io::deserializeDocument(bad.data(), bad.size());
    CHECK(!r.ok);
    CHECK(!r.error.empty());
    auto trunc = io::deserializeDocument(bytes.data(), 40);
    CHECK(!trunc.ok);
    const char junk[] = "hola";
    CHECK(!io::deserializeDocument(reinterpret_cast<const std::uint8_t*>(junk), 4).ok);
}

TEST("guardado atomico, carga y miniatura") {
    auto dir = std::filesystem::temp_directory_path() / "xeldi_test";
    std::filesystem::create_directories(dir);
    std::string path = (dir / "proyecto.xeldi").string();
    std::string err;
    CHECK(io::saveDocument(sample(), path, &err));
    CHECK(!std::filesystem::exists(path + ".tmp"));
    auto r = io::loadDocument(path);
    CHECK(r.ok);
    auto th = io::readThumbnail(path);
    CHECK(th && th->width() <= 128 && th->width() > 0);
}

TEST("exportar OBJ y spritesheet") {
    VoxelGrid g;
    g.set({0, 0, 0}, {0xFF0000FFu});
    g.set({1, 0, 0}, {0x00FF00FFu});
    std::string obj, mtl;
    auto st = io::buildObj(g, "m.mtl", obj, mtl);
    CHECK_EQ(int(st.faces), 10);  // 12 caras menos las 2 internas
    CHECK_EQ(int(st.materials), 2);
    CHECK(obj.find("mtllib m.mtl") != std::string::npos);
    Document d = sample();
    auto sheet = io::buildSpritesheet(d, {0, -1, 3, 1, 2});
    CHECK_EQ(sheet.image.width(), 3 * 64 + 2);
    CHECK_EQ(sheet.image.height(), 48);
    CHECK(sheet.json.find("\"idle\"") != std::string::npos);
}

TEST_MAIN()
