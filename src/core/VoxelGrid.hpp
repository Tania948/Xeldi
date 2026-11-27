#pragma once
// Rejilla dispersa de voxeles en chunks de 16x16x16.
// 1 unidad del mundo = 1 voxel = 1 pixel del lienzo 2D. Eje Y hacia arriba.
#include <array>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include "core/Color.hpp"
#include "core/Math.hpp"

namespace xeldi {

struct Voxel {
    Rgba rgba = kTransparent;  ///< alfa 0 = vacio
    std::uint8_t flags = 0;
    bool empty() const { return alpha(rgba) == 0; }
    bool operator==(const Voxel&) const = default;
};

constexpr int kChunkSize = 16;
constexpr int kChunkVolume = kChunkSize * kChunkSize * kChunkSize;

struct VoxelChunk {
    std::array<Voxel, kChunkVolume> voxels{};
    int count = 0;  ///< voxeles no vacios
    static int index(int lx, int ly, int lz) { return (lz * kChunkSize + ly) * kChunkSize + lx; }
};

class VoxelGrid {
public:
    using ChunkMap = std::unordered_map<IVec3, std::unique_ptr<VoxelChunk>, IVec3Hash>;

    VoxelGrid() = default;
    VoxelGrid(const VoxelGrid& o);
    VoxelGrid& operator=(const VoxelGrid& o);
    VoxelGrid(VoxelGrid&&) noexcept = default;
    VoxelGrid& operator=(VoxelGrid&&) noexcept = default;

    Voxel get(IVec3 p) const;
    bool has(IVec3 p) const { return !get(p).empty(); }
    /// Coloca un voxel (vacio = borrar). Devuelve el valor anterior.
    Voxel set(IVec3 p, Voxel v);
    void clear() { chunks_.clear(); }

    std::size_t voxelCount() const;
    std::size_t chunkCount() const { return chunks_.size(); }
    bool empty() const { return chunks_.empty(); }
    /// Caja envolvente [min, max] inclusiva. false si no hay voxeles.
    bool bounds(IVec3& min, IVec3& max) const;

    const ChunkMap& chunks() const { return chunks_; }
    /// Acceso para el deserializador: reemplaza un chunk completo.
    void setChunk(IVec3 chunkCoord, std::unique_ptr<VoxelChunk> chunk);

    template <class F>
    void forEach(F&& f) const {
        for (const auto& [cc, chunk] : chunks_) {
            for (int z = 0; z < kChunkSize; ++z)
                for (int y = 0; y < kChunkSize; ++y)
                    for (int x = 0; x < kChunkSize; ++x) {
                        const Voxel& v = chunk->voxels[VoxelChunk::index(x, y, z)];
                        if (!v.empty())
                            f(IVec3{cc.x * kChunkSize + x, cc.y * kChunkSize + y, cc.z * kChunkSize + z}, v);
                    }
        }
    }

    static int floorDiv(int a, int b) { return (a >= 0) ? a / b : -((-a + b - 1) / b); }
    static IVec3 chunkOf(IVec3 p) {
        return {floorDiv(p.x, kChunkSize), floorDiv(p.y, kChunkSize), floorDiv(p.z, kChunkSize)};
    }

    bool operator==(const VoxelGrid& o) const;

private:
    ChunkMap chunks_;
};

}  // namespace xeldi
