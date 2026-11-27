#include "core/Commands.hpp"
#include <algorithm>

namespace xeldi {

// ---------------- PixelPatchCommand ----------------
PixelPatchCommand::PixelPatchCommand(std::string label, std::string layerId, int frame, Rect r, Image before,
                                     Image after, bool celWasEmpty)
    : label_(std::move(label)), layerId_(std::move(layerId)), frame_(frame), rect_(r),
      before_(std::move(before)), after_(std::move(after)), celWasEmpty_(celWasEmpty) {}

void PixelPatchCommand::undo(Document& doc) {
    int l = doc.layerIndex(layerId_);
    if (l < 0 || frame_ >= doc.frameCount()) return;
    if (celWasEmpty_) {
        doc.layers[l].cels[frame_].reset();
        return;
    }
    doc.ensureCel(l, frame_).paste(before_, rect_.x, rect_.y);
}

void PixelPatchCommand::redo(Document& doc) {
    int l = doc.layerIndex(layerId_);
    if (l < 0 || frame_ >= doc.frameCount()) return;
    doc.ensureCel(l, frame_).paste(after_, rect_.x, rect_.y);
}

std::size_t PixelPatchCommand::memoryBytes() const {
    return sizeof(*this) + (before_.pixels().size() + after_.pixels().size()) * sizeof(Rgba);
}

// ---------------- VoxelEditCommand ----------------
VoxelEditCommand::VoxelEditCommand(std::string label, std::vector<VoxelChange> changes)
    : label_(std::move(label)), changes_(std::move(changes)) {}

void VoxelEditCommand::undo(Document& doc) {
    for (auto it = changes_.rbegin(); it != changes_.rend(); ++it) doc.scene.voxels.set(it->pos, it->before);
}
void VoxelEditCommand::redo(Document& doc) {
    for (const auto& c : changes_) doc.scene.voxels.set(c.pos, c.after);
}

// ---------------- WorkplaneImageCommand ----------------
WorkplaneImageCommand::WorkplaneImageCommand(std::string label, std::string planeId, Image before, Image after)
    : label_(std::move(label)), planeId_(std::move(planeId)), before_(std::move(before)), after_(std::move(after)) {}

void WorkplaneImageCommand::undo(Document& doc) {
    if (auto* p = doc.scene.findPlane(planeId_)) p->pixels = before_;
}
void WorkplaneImageCommand::redo(Document& doc) {
    if (auto* p = doc.scene.findPlane(planeId_)) p->pixels = after_;
}
std::size_t WorkplaneImageCommand::memoryBytes() const {
    return (before_.pixels().size() + after_.pixels().size()) * sizeof(Rgba);
}

// ---------------- Structure ----------------
StructureState StructureState::capture(const Document& d) {
    return {d.layers, d.timeline, d.onion, d.palette, d.width, d.height};
}
void StructureState::apply(Document& d) const {
    d.layers = layers;
    d.timeline = timeline;
    d.onion = onion;
    d.palette = palette;
    d.width = width;
    d.height = height;
}

StructureCommand::StructureCommand(std::string label, StructureState before, StructureState after, unsigned flags)
    : label_(std::move(label)), before_(std::move(before)), after_(std::move(after)), flags_(flags) {}

SceneLayoutState SceneLayoutState::capture(const Document& d) {
    return {d.scene.workplanes, d.scene.activeWorkplane};
}
void SceneLayoutState::apply(Document& d) const {
    d.scene.workplanes = workplanes;
    d.scene.activeWorkplane = activeWorkplane;
}

SceneLayoutCommand::SceneLayoutCommand(std::string label, SceneLayoutState before, SceneLayoutState after)
    : label_(std::move(label)), before_(std::move(before)), after_(std::move(after)) {}

std::size_t SceneLayoutCommand::memoryBytes() const {
    std::size_t n = 0;
    for (const auto& w : before_.workplanes) n += w.pixels.pixels().size() * 4;
    for (const auto& w : after_.workplanes) n += w.pixels.pixels().size() * 4;
    return n;
}

// ---------------- Macro ----------------
void MacroCommand::undo(Document& doc) {
    for (auto it = cmds_.rbegin(); it != cmds_.rend(); ++it) (*it)->undo(doc);
}
void MacroCommand::redo(Document& doc) {
    for (auto& c : cmds_) c->redo(doc);
}
unsigned MacroCommand::changes() const {
    unsigned f = 0;
    for (const auto& c : cmds_) f |= c->changes();
    return f;
}
std::size_t MacroCommand::memoryBytes() const {
    std::size_t n = 0;
    for (const auto& c : cmds_) n += c->memoryBytes();
    return n;
}

// ---------------- PixelStroke ----------------
PixelStroke::PixelStroke(Document& doc, int layer, int frame)
    : doc_(doc), layer_(layer), frame_(frame), celWasEmpty_(doc.cel(layer, frame) == nullptr),
      target_(&doc.ensureCel(layer, frame)), before_(*target_) {}

void PixelStroke::restore() {
    if (dirty_.isEmpty()) return;
    target_->paste(before_.copy(dirty_), dirty_.x, dirty_.y);
}

std::unique_ptr<Command> PixelStroke::finish(const std::string& label) {
    // Recorta el rectangulo sucio a los pixeles que realmente cambiaron.
    int x0 = dirty_.x + dirty_.w, y0 = dirty_.y + dirty_.h, x1 = -1, y1 = -1;
    for (int y = dirty_.y; y < dirty_.y + dirty_.h; ++y)
        for (int x = dirty_.x; x < dirty_.x + dirty_.w; ++x)
            if (target_->pixel(x, y) != before_.pixel(x, y)) {
                x0 = std::min(x0, x); y0 = std::min(y0, y);
                x1 = std::max(x1, x); y1 = std::max(y1, y);
            }
    if (x1 < 0) {
        if (celWasEmpty_) doc_.layers[layer_].cels[frame_].reset();
        return nullptr;
    }
    Rect r{x0, y0, x1 - x0 + 1, y1 - y0 + 1};
    return std::make_unique<PixelPatchCommand>(label, doc_.layers[layer_].id, frame_, r, before_.copy(r),
                                               target_->copy(r), celWasEmpty_);
}

// ---------------- VoxelStroke ----------------
void VoxelStroke::set(IVec3 p, Voxel v) {
    Voxel old = doc_.scene.voxels.set(p, v);
    if (before_.emplace(p, old).second) order_.push_back(p);
}

std::unique_ptr<Command> VoxelStroke::finish(const std::string& label) {
    std::vector<VoxelChange> ch;
    for (const IVec3& p : order_) {
        Voxel b = before_[p], a = doc_.scene.voxels.get(p);
        if (!(a == b)) ch.push_back({p, b, a});
    }
    before_.clear();
    order_.clear();
    if (ch.empty()) return nullptr;
    return std::make_unique<VoxelEditCommand>(label, std::move(ch));
}

void VoxelStroke::cancel() {
    for (auto it = order_.rbegin(); it != order_.rend(); ++it) doc_.scene.voxels.set(*it, before_[*it]);
    before_.clear();
    order_.clear();
}

// ---------------- PlaneStroke ----------------
PlaneStroke::PlaneStroke(Document& doc, int planeIndex) : doc_(doc) {
    if (planeIndex >= 0 && planeIndex < int(doc.scene.workplanes.size())) {
        auto& p = doc.scene.workplanes[planeIndex];
        p.ensurePixels();
        planeId_ = p.id;
        before_ = p.pixels;
    }
}

Image* PlaneStroke::target() {
    auto* p = doc_.scene.findPlane(planeId_);
    return p ? &p->pixels : nullptr;
}

std::unique_ptr<Command> PlaneStroke::finish(const std::string& label) {
    auto* t = target();
    if (!t || *t == before_) return nullptr;
    return std::make_unique<WorkplaneImageCommand>(label, planeId_, before_, *t);
}

}  // namespace xeldi
