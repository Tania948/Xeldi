#include "Test.hpp"
#include "core/VoxelGrid.hpp"

using namespace xeldi;

TEST("set/get en coordenadas negativas y chunks") {
    VoxelGrid g;
    g.set({-1, -1, -1}, {0xFF0000FFu});
    g.set({0, 0, 0}, {0x00FF00FFu});
    g.set({16, 0, 0}, {0x0000FFFFu});
    CHECK(g.get({-1, -1, -1}).rgba == 0xFF0000FFu);
    CHECK(g.get({0, 0, 0}).rgba == 0x00FF00FFu);
    CHECK(g.has({16, 0, 0}));
    CHECK(!g.has({1, 0, 0}));
    CHECK_EQ(int(g.chunkCount()), 3);
    CHECK_EQ(int(g.voxelCount()), 3);
    CHECK(VoxelGrid::chunkOf({-1, 0, 15}) == (IVec3{-1, 0, 0}));
    CHECK(VoxelGrid::chunkOf({-16, 0, 16}) == (IVec3{-1, 0, 1}));
    CHECK(VoxelGrid::chunkOf({-17, 0, 0}) == (IVec3{-2, 0, 0}));
}

TEST("borrar libera chunks vacios (almacenamiento disperso)") {
    VoxelGrid g;
    g.set({5, 5, 5}, {0xFFFFFFFFu});
    Voxel old = g.set({5, 5, 5}, {});
    CHECK(old.rgba == 0xFFFFFFFFu);
    CHECK(g.empty());
    // Un mundo "grande" con pocos voxeles ocupa pocos chunks.
    for (int i = 0; i < 1000; i += 100) g.set({i, i, i}, {0x123456FFu});
    CHECK_EQ(int(g.chunkCount()), 10);
}

TEST("bounds y copia profunda") {
    VoxelGrid g;
    g.set({-3, 2, 7}, {0x1u | 0xFF000000u});
    g.set({4, -2, 0}, {0x2u | 0xFF000000u});
    IVec3 mn, mx;
    CHECK(g.bounds(mn, mx));
    CHECK(mn == (IVec3{-3, -2, 0}));
    CHECK(mx == (IVec3{4, 2, 7}));
    VoxelGrid c = g;
    c.set({-3, 2, 7}, {});
    CHECK(g.has({-3, 2, 7}));
    CHECK(!(c == g));
}

TEST_MAIN()
