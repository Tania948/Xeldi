#include "io/MeshExporter.hpp"

#include <array>
#include <cstdio>
#include <tuple>
#include <vector>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>

namespace xeldi::io {

namespace {
struct Face {
    IVec3 n;
    int c[4][3];
};
const Face kFaces[6] = {
    {{1, 0, 0}, {{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}},
    {{-1, 0, 0}, {{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}}},
    {{0, 1, 0}, {{0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0}}},
    {{0, -1, 0}, {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}},
    {{0, 0, 1}, {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}},
    {{0, 0, -1}, {{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}}},
};
std::string matName(Rgba c) {
    char b[16];
    std::snprintf(b, sizeof b, "c%02x%02x%02x", red(c), green(c), blue(c));
    return b;
}
}  // namespace

MeshStats buildObj(const VoxelGrid& grid, const std::string& mtlFileName, std::string& objOut, std::string& mtlOut) {
    MeshStats st;
    std::map<IVec3, int, bool (*)(const IVec3&, const IVec3&)> verts(
        [](const IVec3& a, const IVec3& b) { return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z); });
    std::map<Rgba, std::vector<std::array<int, 4>>> facesByColor;
    std::ostringstream vtx;
    auto vid = [&](IVec3 p) {
        auto it = verts.find(p);
        if (it != verts.end()) return it->second;
        int id = int(verts.size()) + 1;
        verts.emplace(p, id);
        vtx << "v " << p.x << ' ' << p.y << ' ' << p.z << '\n';
        return id;
    };
    grid.forEach([&](IVec3 p, const Voxel& v) {
        for (const Face& f : kFaces) {
            if (grid.has(p + f.n)) continue;
            std::array<int, 4> ids{};
            for (int i = 0; i < 4; ++i) ids[i] = vid(p + IVec3{f.c[i][0], f.c[i][1], f.c[i][2]});
            facesByColor[withAlpha(v.rgba, 255)].push_back(ids);
            ++st.faces;
        }
    });
    std::ostringstream obj, mtl;
    obj << "# Exportado por Xeldi " << XELDI_VERSION << "\n";
    obj << "mtllib " << mtlFileName << "\n";
    obj << vtx.str();
    obj << "vn 1 0 0\nvn -1 0 0\nvn 0 1 0\nvn 0 -1 0\nvn 0 0 1\nvn 0 0 -1\n";
    mtl << "# Materiales de Xeldi\n";
    for (const auto& [col, faces] : facesByColor) {
        std::string name = matName(col);
        mtl << "newmtl " << name << "\nKd " << red(col) / 255.0 << ' ' << green(col) / 255.0 << ' '
            << blue(col) / 255.0 << "\nKa 0 0 0\nKs 0 0 0\nd 1\nillum 1\n\n";
        obj << "usemtl " << name << "\n";
        for (const auto& f : faces) obj << "f " << f[0] << ' ' << f[1] << ' ' << f[2] << ' ' << f[3] << '\n';
    }
    st.vertices = verts.size();
    st.materials = facesByColor.size();
    objOut = obj.str();
    mtlOut = mtl.str();
    return st;
}

bool exportObj(const VoxelGrid& grid, const std::string& objPath, MeshStats* stats, std::string* error) {
    namespace fs = std::filesystem;
    fs::path p(objPath);
    fs::path mtlPath = p;
    mtlPath.replace_extension(".mtl");
    std::string obj, mtl;
    MeshStats st = buildObj(grid, mtlPath.filename().string(), obj, mtl);
    std::ofstream fo(p), fm(mtlPath);
    if (!fo || !fm) {
        if (error) *error = "no se pudo escribir " + objPath;
        return false;
    }
    fo << obj;
    fm << mtl;
    if (stats) *stats = st;
    return bool(fo) && bool(fm);
}

}  // namespace xeldi::io
