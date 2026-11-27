#pragma once
// Comandos concretos y "grabadores" de trazos que los generan.
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/Command.hpp"
#include "core/Document.hpp"

namespace xeldi {

/// Parche de pixeles en una cel: solo guarda el rectangulo modificado.
class PixelPatchCommand : public Command {
public:
    PixelPatchCommand(std::string label, std::string layerId, int frame, Rect r, Image before, Image after,
                      bool celWasEmpty);
    std::string label() const override { return label_; }
    void undo(Document& doc) override;
    void redo(Document& doc) override;
    unsigned changes() const override { return ChangePixels; }
    std::size_t memoryBytes() const override;
    const Rect& rect() const { return rect_; }

private:
    std::string label_, layerId_;
    int frame_;
    Rect rect_;
    Image before_, after_;
    bool celWasEmpty_;
};

struct VoxelChange {
    IVec3 pos;
    Voxel before, after;
};

class VoxelEditCommand : public Command {
public:
    VoxelEditCommand(std::string label, std::vector<VoxelChange> changes);
    std::string label() const override { return label_; }
    void undo(Document& doc) override;
    void redo(Document& doc) override;
    unsigned changes() const override { return ChangeScene3D; }
    std::size_t memoryBytes() const override { return changes_.size() * sizeof(VoxelChange); }
    std::size_t size() const { return changes_.size(); }

private:
    std::string label_;
    std::vector<VoxelChange> changes_;
};

/// Cambio del bitmap de un plano de trabajo.
class WorkplaneImageCommand : public Command {
public:
    WorkplaneImageCommand(std::string label, std::string planeId, Image before, Image after);
    std::string label() const override { return label_; }
    void undo(Document& doc) override;
    void redo(Document& doc) override;
    unsigned changes() const override { return ChangeScene3D; }
    std::size_t memoryBytes() const override;

private:
    std::string label_, planeId_;
    Image before_, after_;
};

/// Estado "estructural" (capas, frames, tags, ajustes). Las cels son shared_ptr,
/// asi que copiar esto es barato: solo se copian punteros.
struct StructureState {
    std::vector<Layer> layers;
    Timeline timeline;
    OnionSkinSettings onion;
    Palette palette;
    int width = 0, height = 0;
    static StructureState capture(const Document& d);
    void apply(Document& d) const;
};

class StructureCommand : public Command {
public:
    StructureCommand(std::string label, StructureState before, StructureState after, unsigned flags);
    std::string label() const override { return label_; }
    void undo(Document& doc) override { before_.apply(doc); }
    void redo(Document& doc) override { after_.apply(doc); }
    unsigned changes() const override { return flags_; }

private:
    std::string label_;
    StructureState before_, after_;
    unsigned flags_;
};

/// Planos de trabajo + camara (no incluye voxeles).
struct SceneLayoutState {
    std::vector<Workplane> workplanes;
    int activeWorkplane = 0;
    static SceneLayoutState capture(const Document& d);
    void apply(Document& d) const;
};

class SceneLayoutCommand : public Command {
public:
    SceneLayoutCommand(std::string label, SceneLayoutState before, SceneLayoutState after);
    std::string label() const override { return label_; }
    void undo(Document& doc) override { before_.apply(doc); }
    void redo(Document& doc) override { after_.apply(doc); }
    unsigned changes() const override { return ChangeScene3D; }
    std::size_t memoryBytes() const override;

private:
    std::string label_;
    SceneLayoutState before_, after_;
};

/// Agrupa varios comandos en un solo paso de deshacer.
class MacroCommand : public Command {
public:
    explicit MacroCommand(std::string label) : label_(std::move(label)) {}
    void add(std::unique_ptr<Command> c) { if (c) cmds_.push_back(std::move(c)); }
    bool empty() const { return cmds_.empty(); }
    std::string label() const override { return label_; }
    void undo(Document& doc) override;
    void redo(Document& doc) override;
    unsigned changes() const override;
    std::size_t memoryBytes() const override;

private:
    std::string label_;
    std::vector<std::unique_ptr<Command>> cmds_;
};

// ---------------------------------------------------------------------------
// Grabadores: se usan mientras dura un trazo y al final producen el Command.
// ---------------------------------------------------------------------------

/// Trazo 2D sobre una cel. Copia la cel al inicio (temporal) y al terminar
/// guarda solo el rectangulo que realmente cambio.
class PixelStroke {
public:
    PixelStroke(Document& doc, int layer, int frame);
    Image& target() { return *target_; }
    void markDirty(const Rect& r) { dirty_ = dirty_.united(r.intersected(target_->rect())); }
    /// Vuelve al estado del inicio del trazo (para previsualizar lineas/rectangulos).
    void restore();
    /// Devuelve un pixel a su color del inicio del trazo.
    void restorePixel(int x, int y) { target_->setPixel(x, y, before_.pixel(x, y)); }
    /// Termina el trazo. nullptr si no hubo cambios.
    std::unique_ptr<Command> finish(const std::string& label);

private:
    Document& doc_;
    int layer_, frame_;
    bool celWasEmpty_;
    Image* target_;
    Image before_;
    Rect dirty_;
};

class VoxelStroke {
public:
    explicit VoxelStroke(Document& doc) : doc_(doc) {}
    void set(IVec3 p, Voxel v);
    std::size_t touched() const { return before_.size(); }
    bool touches(IVec3 p) const { return before_.count(p) != 0; }
    std::unique_ptr<Command> finish(const std::string& label);
    /// Deshace los cambios en curso sin registrarlos.
    void cancel();

private:
    Document& doc_;
    std::unordered_map<IVec3, Voxel, IVec3Hash> before_;
    std::vector<IVec3> order_;
};

class PlaneStroke {
public:
    PlaneStroke(Document& doc, int planeIndex);
    Image* target();
    std::unique_ptr<Command> finish(const std::string& label);

private:
    Document& doc_;
    std::string planeId_;
    Image before_;
};

}  // namespace xeldi
