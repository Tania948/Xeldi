#include "core/VoxelGrid.hpp"

#include <algorithm>
#include <climits>

namespace xeldi {

VoxelGrid::VoxelGrid(const VoxelGrid& o) { *this = o; }

VoxelGrid& VoxelGrid::operator=(const VoxelGrid& o) {
    if (this == &o) return *this;
    chunks_.clear();
    for (const auto& [k, c] : o.chunks_) chunks_[k] = std::make_unique<VoxelChunk>(*c);
    return *this;
}

Voxel VoxelGrid::get(IVec3 p) const {
    auto it = chunks_.find(chunkOf(p));
    if (it == chunks_.end()) return {};
    IVec3 c = it->first;
    return it->second->voxels[VoxelChunk::index(p.x - c.x * kChunkSize, p.y - c.y * kChunkSize,
                                                p.z - c.z * kChunkSize)];
}

Voxel VoxelGrid::set(IVec3 p, Voxel v) {
    if (v.empty()) v = {};
    IVec3 cc = chunkOf(p);
    auto it = chunks_.find(cc);
    if (it == chunks_.end()) {
        if (v.empty()) return {};
        it = chunks_.emplace(cc, std::make_unique<VoxelChunk>()).first;
    }
    VoxelChunk& chunk = *it->second;
    Voxel& slot = chunk.voxels[VoxelChunk::index(p.x - cc.x * kChunkSize, p.y - cc.y * kChunkSize,
                                                 p.z - cc.z * kChunkSize)];
    Voxel old = slot;
    if (old.empty() && !v.empty()) ++chunk.count;
    if (!old.empty() && v.empty()) --chunk.count;
    slot = v;
    if (chunk.count == 0) chunks_.erase(it);
    return old;
}

std::size_t VoxelGrid::voxelCount() const {
    std::size_t n = 0;
    for (const auto& [k, c] : chunks_) n += std::size_t(c->count);
    return n;
}

bool VoxelGrid::bounds(IVec3& mn, IVec3& mx) const {
    if (chunks_.empty()) return false;
    mn = {INT_MAX, INT_MAX, INT_MAX};
    mx = {INT_MIN, INT_MIN, INT_MIN};
    forEach([&](IVec3 p, const Voxel&) {
        mn = {std::min(mn.x, p.x), std::min(mn.y, p.y), std::min(mn.z, p.z)};
        mx = {std::max(mx.x, p.x), std::max(mx.y, p.y), std::max(mx.z, p.z)};
    });
    return true;
}

void VoxelGrid::setChunk(IVec3 cc, std::unique_ptr<VoxelChunk> chunk) {
    if (!chunk) { chunks_.erase(cc); return; }
    chunk->count = int(std::count_if(chunk->voxels.begin(), chunk->voxels.end(),
                                     [](const Voxel& v) { return !v.empty(); }));
    if (chunk->count == 0) { chunks_.erase(cc); return; }
    chunks_[cc] = std::move(chunk);
}

bool VoxelGrid::operator==(const VoxelGrid& o) const {
    if (chunks_.size() != o.chunks_.size()) return false;
    for (const auto& [k, c] : chunks_) {
        auto it = o.chunks_.find(k);
        if (it == o.chunks_.end() || it->second->voxels != c->voxels) return false;
    }
    return true;
}

}  // namespace xeldi
