#include "editor/Editor.hpp"

#include <algorithm>
#include <cmath>

#include "engine2d/Compositor.hpp"
#include "engine3d/Picking.hpp"
#include "engine3d/Voxelize.hpp"

namespace xeldi {

const char* toolId(Tool2D t) {
    switch (t) {
        case Tool2D::Pencil: return "tool.pencil";
        case Tool2D::Eraser: return "tool.eraser";
        case Tool2D::Line: return "tool.line";
        case Tool2D::Rectangle: return "tool.rect";
        case Tool2D::FilledRectangle: return "tool.rect_filled";
        case Tool2D::Ellipse: return "tool.ellipse";
        case Tool2D::Fill: return "tool.fill";
        case Tool2D::Picker: return "tool.picker";
    }
    return "tool.pencil";
}

const char* toolId(Tool3D t) {
    switch (t) {
        case Tool3D::PlaceVoxel: return "tool3d.place";
        case Tool3D::EraseVoxel: return "tool3d.erase";
        case Tool3D::PaintVoxel: return "tool3d.paint";
        case Tool3D::DrawOnPlane: return "tool3d.draw_plane";
        case Tool3D::EraseOnPlane: return "tool3d.erase_plane";
        case Tool3D::PickColor: return "tool3d.pick";
    }
    return "tool3d.place";
}

Editor::Editor() : doc_(Document::createBlank(64, 64)) { registerActions(); }

// ---------------------------------------------------------------------------
void Editor::setDocument(Document d, std::string path) {
    cancelStroke2D();
    endStroke3D();
    doc_ = std::move(d);
    path_ = std::move(path);
    history_.clear();
    playing_ = false;
    frame_ = 0;
    layer_ = std::max(0, int(doc_.layers.size()) - 1);
    clampSelection();
    notify(ChangeAll);
}

void Editor::commit(std::unique_ptr<Command> cmd) {
    if (!cmd) return;
    unsigned f = cmd->changes();
    history_.push(std::move(cmd));
    notify(f | ChangeHistory);
}

bool Editor::undo() {
    if (stroke2d_ || voxelStroke_ || planeStroke_) return false;
    unsigned f = history_.undo(doc_);
    if (!f) return false;
    clampSelection();
    notify(f);
    return true;
}

bool Editor::redo() {
    if (stroke2d_ || voxelStroke_ || planeStroke_) return false;
    unsigned f = history_.redo(doc_);
    if (!f) return false;
    clampSelection();
    notify(f);
    return true;
}

void Editor::editStructure(const std::string& label, unsigned flags, const std::function<void(Document&)>& fn) {
    auto before = StructureState::capture(doc_);
    fn(doc_);
    auto after = StructureState::capture(doc_);
    history_.push(std::make_unique<StructureCommand>(label, std::move(before), std::move(after), flags));
    clampSelection();
    notify(flags | ChangeHistory);
}

void Editor::editSceneLayout(const std::string& label, const std::function<void(Document&)>& fn) {
    auto before = SceneLayoutState::capture(doc_);
    fn(doc_);
    auto after = SceneLayoutState::capture(doc_);
    history_.push(std::make_unique<SceneLayoutCommand>(label, std::move(before), std::move(after)));
    notify(ChangeScene3D | ChangeHistory);
}

void Editor::clampSelection() {
    frame_ = std::clamp(frame_, 0, std::max(0, doc_.frameCount() - 1));
    layer_ = std::clamp(layer_, 0, std::max(0, int(doc_.layers.size()) - 1));
    auto& sc = doc_.scene;
    sc.activeWorkplane = std::clamp(sc.activeWorkplane, 0, std::max(0, int(sc.workplanes.size()) - 1));
}

void Editor::setCurrentFrame(int f) {
    f = std::clamp(f, 0, doc_.frameCount() - 1);
    if (f == frame_) return;
    cancelStroke2D();
    frame_ = f;
    notify(ChangeSelection);
}

void Editor::setCurrentLayer(int l) {
    l = std::clamp(l, 0, int(doc_.layers.size()) - 1);
    if (l == layer_) return;
    cancelStroke2D();
    layer_ = l;
    notify(ChangeSelection);
}

void Editor::setPrimaryColor(Rgba c) { primary_ = c; notify(ChangeSelection); }
void Editor::setSecondaryColor(Rgba c) { secondary_ = c; notify(ChangeSelection); }
void Editor::swapColors() { std::swap(primary_, secondary_); notify(ChangeSelection); }
void Editor::setTool2D(Tool2D t) { tool2d_ = t; notify(ChangeSelection); }
void Editor::setTool3D(Tool3D t) { tool3d_ = t; notify(ChangeSelection); }
void Editor::setBrushSize(int s) { brush_.size = std::clamp(s, 1, 64); notify(ChangeSelection); }

// ---------------------------------------------------------------------------
// Trazos 2D
// ---------------------------------------------------------------------------
static Point toPoint(const PointerInput& p) { return {int(std::floor(p.x)), int(std::floor(p.y))}; }

Rgba Editor::sampleColor(int x, int y) const { return composeFrame(doc_, frame_).pixel(x, y); }

void Editor::beginStroke2D(const PointerInput& in) {
    cancelStroke2D();
    if (layer_ < 0 || layer_ >= int(doc_.layers.size())) return;
    Point p = toPoint(in);
    if (tool2d_ == Tool2D::Picker) {
        Rgba c = sampleColor(p.x, p.y);
        in.primary ? setPrimaryColor(c) : setSecondaryColor(c);
        return;
    }
    const Layer& L = doc_.layers[layer_];
    if (L.locked || !L.visible || L.type != LayerType::Raster2D) return;
    strokeColor_ = tool2d_ == Tool2D::Eraser ? kTransparent : (in.primary ? primary_ : secondary_);
    stroke2d_ = std::make_unique<PixelStroke>(doc_, layer_, frame_);
    strokeStart_ = strokeLast_ = p;
    strokePath_ = {p};
    if (tool2d_ == Tool2D::Fill) {
        stroke2d_->markDirty(floodFill(stroke2d_->target(), p, strokeColor_, true));
        endStroke2D(in);
        return;
    }
    applyTool2D(p, p, false);
}

void Editor::applyTool2D(Point a, Point b, bool preview) {
    Image& img = stroke2d_->target();
    switch (tool2d_) {
        case Tool2D::Pencil:
        case Tool2D::Eraser: {
            for (const Point& q : bresenhamLine(a, b)) {
                if (!strokePath_.empty() && strokePath_.back() == q && !(a == b && strokePath_.size() == 1)) continue;
                // Pixel perfect: quita la esquina en L que forman los ultimos tres puntos.
                if (pixelPerfect_ && brush_.size == 1 && strokePath_.size() >= 2) {
                    Point p0 = strokePath_[strokePath_.size() - 2], p1 = strokePath_.back();
                    bool adj0 = std::abs(p0.x - p1.x) + std::abs(p0.y - p1.y) == 1;
                    bool adj1 = std::abs(q.x - p1.x) + std::abs(q.y - p1.y) == 1;
                    if (adj0 && adj1 && p0.x != q.x && p0.y != q.y) {
                        stroke2d_->restorePixel(p1.x, p1.y);
                        strokePath_.pop_back();
                    }
                }
                if (strokePath_.empty() || !(strokePath_.back() == q)) strokePath_.push_back(q);
                stroke2d_->markDirty(stampBrush(img, q, strokeColor_, brush_));
            }
            break;
        }
        case Tool2D::Line:
            if (preview) stroke2d_->restore();
            stroke2d_->markDirty(drawLine(img, strokeStart_, b, strokeColor_, brush_));
            break;
        case Tool2D::Rectangle:
        case Tool2D::FilledRectangle:
            if (preview) stroke2d_->restore();
            stroke2d_->markDirty(
                drawRectangle(img, strokeStart_, b, strokeColor_, tool2d_ == Tool2D::FilledRectangle, brush_));
            break;
        case Tool2D::Ellipse:
            if (preview) stroke2d_->restore();
            stroke2d_->markDirty(drawEllipse(img, strokeStart_, b, strokeColor_, false));
            break;
        default: break;
    }
}

void Editor::moveStroke2D(const PointerInput& in) {
    if (!stroke2d_) return;
    Point p = toPoint(in);
    if (p == strokeLast_) return;
    applyTool2D(strokeLast_, p, true);
    strokeLast_ = p;
    notify(ChangePixels);
}

void Editor::endStroke2D(const PointerInput&) {
    if (!stroke2d_) return;
    static const char* labels[] = {"Lapiz", "Borrador", "Linea", "Rectangulo", "Rectangulo relleno",
                                   "Elipse", "Relleno", "Cuentagotas"};
    auto cmd = stroke2d_->finish(labels[int(tool2d_)]);
    stroke2d_.reset();
    strokePath_.clear();
    if (cmd) commit(std::move(cmd));
    else notify(ChangePixels);
}

void Editor::cancelStroke2D() {
    if (!stroke2d_) return;
    stroke2d_->restore();
    stroke2d_->finish("");  // limpia la cel si estaba vacia
    stroke2d_.reset();
    strokePath_.clear();
    notify(ChangePixels);
}

// ---------------------------------------------------------------------------
// Edicion 3D
// ---------------------------------------------------------------------------
Ray Editor::rayFor(const PointerInput& p, int vw, int vh) const {
    return screenRay(doc_.scene.camera, vw, vh, p.x, p.y);
}

static std::optional<IVec3> groundCell(const Ray& r) {
    auto t = intersectPlane({0, 0, 0}, {0, 1, 0}, r);
    if (!t) return std::nullopt;
    return floorToCell(r.at(*t) + Vec3{0, 0.5f, 0});
}

std::optional<IVec3> Editor::hoverCell(float sx, float sy, int vw, int vh) const {
    Ray r = screenRay(doc_.scene.camera, vw, vh, sx, sy);
    auto hit = raycastVoxels(doc_.scene.voxels, r);
    switch (tool3d_) {
        case Tool3D::PlaceVoxel: {
            if (hit) return hit->adjacent();
            auto& sc = const_cast<Scene3D&>(doc_.scene);
            if (auto* pl = sc.activePlane(); pl && pl->visible)
                if (auto c = cellOnPlane(*pl, r)) return c;
            return groundCell(r);
        }
        case Tool3D::EraseVoxel:
        case Tool3D::PaintVoxel:
        case Tool3D::PickColor:
            if (hit) return hit->cell;
            return std::nullopt;
        default: return std::nullopt;
    }
}

void Editor::applyTool3D(const PointerInput& in, int vw, int vh) {
    Ray r = rayFor(in, vw, vh);
    auto& sc = doc_.scene;
    switch (tool3d_) {
        case Tool3D::PlaceVoxel:
        case Tool3D::EraseVoxel:
        case Tool3D::PaintVoxel: {
            if (!voxelStroke_) return;
            bool erase = tool3d_ == Tool3D::EraseVoxel || (tool3d_ == Tool3D::PlaceVoxel && !stroke3dPrimary_);
            auto hit = raycastVoxels(sc.voxels, r);
            std::optional<IVec3> cell;
            if (tool3d_ == Tool3D::PlaceVoxel && !erase) {
                // Al arrastrar no se construye encima de lo que se acaba de poner.
                if (hit && !voxelStroke_->touches(hit->cell)) cell = hit->adjacent();
                else if (!hit || voxelStroke_->touches(hit->cell)) {
                    if (auto* pl = sc.activePlane(); pl && pl->visible) cell = cellOnPlane(*pl, r);
                    if (!cell && !hit) cell = groundCell(r);
                }
            } else if (hit) {
                cell = hit->cell;
            }
            if (!cell || cell == lastCell3d_) return;
            lastCell3d_ = cell;
            if (erase) voxelStroke_->set(*cell, Voxel{});
            else voxelStroke_->set(*cell, Voxel{withAlpha(primary_, 255), 0});
            notify(ChangeScene3D);
            break;
        }
        case Tool3D::DrawOnPlane:
        case Tool3D::EraseOnPlane: {
            if (!planeStroke_) return;
            auto* pl = sc.activePlane();
            Image* img = planeStroke_->target();
            if (!pl || !img) return;
            auto px = pickWorkplane(*pl, r);
            if (!px) { lastPlanePx_.reset(); return; }
            Rgba c = (tool3d_ == Tool3D::EraseOnPlane || !stroke3dPrimary_) ? kTransparent : primary_;
            Point b{px->px, px->py};
            Point a = lastPlanePx_ ? Point{lastPlanePx_->first, lastPlanePx_->second} : b;
            drawLine(*img, a, b, c, brush_);
            lastPlanePx_ = std::make_pair(b.x, b.y);
            notify(ChangeScene3D);
            break;
        }
        case Tool3D::PickColor: {
            if (auto hit = raycastVoxels(sc.voxels, r)) {
                setPrimaryColor(sc.voxels.get(hit->cell).rgba);
            } else if (auto* pl = sc.activePlane()) {
                if (auto px = pickWorkplane(*pl, r)) {
                    Rgba c = pl->pixels.pixel(px->px, px->py);
                    if (alpha(c)) setPrimaryColor(c);
                }
            }
            break;
        }
    }
}

void Editor::beginStroke3D(const PointerInput& p, int vw, int vh) {
    endStroke3D();
    stroke3dPrimary_ = p.primary;
    lastCell3d_.reset();
    lastPlanePx_.reset();
    if (tool3d_ == Tool3D::DrawOnPlane || tool3d_ == Tool3D::EraseOnPlane) {
        if (!doc_.scene.activePlane()) return;
        planeStroke_ = std::make_unique<PlaneStroke>(doc_, doc_.scene.activeWorkplane);
    } else if (tool3d_ != Tool3D::PickColor) {
        voxelStroke_ = std::make_unique<VoxelStroke>(doc_);
    }
    applyTool3D(p, vw, vh);
}

void Editor::moveStroke3D(const PointerInput& p, int vw, int vh) {
    if (!voxelStroke_ && !planeStroke_) return;
    applyTool3D(p, vw, vh);
}

void Editor::endStroke3D() {
    if (voxelStroke_) {
        static const char* labels[] = {"Colocar voxeles", "Borrar voxeles", "Pintar voxeles"};
        int i = std::clamp(int(tool3d_), 0, 2);
        auto cmd = voxelStroke_->finish(labels[i]);
        voxelStroke_.reset();
        if (cmd) commit(std::move(cmd));
    }
    if (planeStroke_) {
        auto cmd = planeStroke_->finish("Dibujar en plano");
        planeStroke_.reset();
        if (cmd) commit(std::move(cmd));
    }
    lastCell3d_.reset();
    lastPlanePx_.reset();
}

// ---------------------------------------------------------------------------
// Operaciones
// ---------------------------------------------------------------------------
void Editor::addLayer() {
    int at = layer_ + 1;
    editStructure("Nueva capa", ChangeLayers, [&](Document& d) {
        d.insertRasterLayer(at, "Capa " + std::to_string(d.layers.size() + 1));
    });
    setCurrentLayer(at);
}

void Editor::duplicateLayer() {
    if (doc_.layers.empty()) return;
    int src = layer_;
    editStructure("Duplicar capa", ChangeLayers, [&](Document& d) {
        Layer copy = d.layers[src];
        copy.id = d.newId("layer");
        copy.name += " copia";
        for (auto& c : copy.cels)
            if (c) c = std::make_shared<Image>(*c);
        d.layers.insert(d.layers.begin() + src + 1, std::move(copy));
    });
    setCurrentLayer(src + 1);
}

void Editor::removeLayer() {
    if (doc_.layers.size() <= 1) return;
    int at = layer_;
    editStructure("Eliminar capa", ChangeLayers, [&](Document& d) { d.layers.erase(d.layers.begin() + at); });
}

void Editor::moveLayer(int delta) {
    int to = layer_ + delta;
    if (to < 0 || to >= int(doc_.layers.size())) return;
    int from = layer_;
    editStructure("Mover capa", ChangeLayers, [&](Document& d) { std::swap(d.layers[from], d.layers[to]); });
    setCurrentLayer(to);
}

void Editor::addFrame() {
    int at = frame_ + 1;
    editStructure("Nuevo frame", ChangeFrames, [&](Document& d) { d.insertFrame(at, d.timeline.frames[frame_]); });
    setCurrentFrame(at);
}

void Editor::duplicateFrame() {
    int at = frame_;
    editStructure("Duplicar frame", ChangeFrames, [&](Document& d) { d.duplicateFrame(at); });
    setCurrentFrame(at + 1);
}

void Editor::removeFrame() {
    if (doc_.frameCount() <= 1) return;
    int at = frame_;
    editStructure("Eliminar frame", ChangeFrames, [&](Document& d) { d.removeFrame(at); });
}

void Editor::addTagFromSelection(const std::string& name, int from, int to) {
    if (from > to) std::swap(from, to);
    from = std::clamp(from, 0, doc_.frameCount() - 1);
    to = std::clamp(to, 0, doc_.frameCount() - 1);
    editStructure("Nuevo tag", ChangeSettings | ChangeFrames, [&](Document& d) {
        AnimationTag t;
        t.name = name;
        t.from = from;
        t.to = to;
        t.autoPreview = d.timeline.tags.empty();
        static const Rgba colors[] = {0xF29BB8FFu, 0xB8C4FFFFu, 0xC9A8E8FFu, 0xFFD8BEFFu, 0xA8E0C8FFu};
        t.color = colors[d.timeline.tags.size() % 5];
        d.timeline.tags.push_back(t);
    });
}

void Editor::addWorkplane(PlaneAxis axis) {
    editSceneLayout("Nuevo plano", [&](Document& d) {
        static const char* names[] = {"Plano frontal", "Plano lateral", "Plano superior"};
        std::string id = d.newId("plane");
        int size = std::max(d.width, d.height);
        auto w = Workplane::axisAligned(axis, 0, size, id, names[int(axis)]);
        w.name += " " + id.substr(id.find('_') + 1);
        d.scene.workplanes.push_back(std::move(w));
        d.scene.activeWorkplane = int(d.scene.workplanes.size()) - 1;
    });
}

void Editor::removeWorkplane() {
    if (doc_.scene.workplanes.empty()) return;
    editSceneLayout("Eliminar plano", [&](Document& d) {
        d.scene.workplanes.erase(d.scene.workplanes.begin() + d.scene.activeWorkplane);
        d.scene.activeWorkplane = std::max(0, d.scene.activeWorkplane - 1);
    });
}

void Editor::nudgeWorkplane(int steps) {
    if (!doc_.scene.activePlane()) return;
    editSceneLayout("Mover plano", [&](Document& d) { d.scene.activePlane()->moveAlongNormal(steps); });
}

void Editor::cycleWorkplane(int delta) {
    int n = int(doc_.scene.workplanes.size());
    if (n == 0) return;
    doc_.scene.activeWorkplane = ((doc_.scene.activeWorkplane + delta) % n + n) % n;
    notify(ChangeScene3D);
}

void Editor::extrudeActivePlane() {
    auto* pl = doc_.scene.activePlane();
    if (!pl) return;
    VoxelStroke s(doc_);
    extrudeWorkplane(*pl, extrudeDepth_, [&](IVec3 p, Voxel v) { s.set(p, v); });
    commit(s.finish("Extruir plano"));
}

void Editor::clearVoxels() {
    VoxelStroke s(doc_);
    std::vector<IVec3> cells;
    doc_.scene.voxels.forEach([&](IVec3 p, const Voxel&) { cells.push_back(p); });
    for (const IVec3& p : cells) s.set(p, Voxel{});
    commit(s.finish("Vaciar voxeles"));
}

void Editor::projectToCel(OrthoView view, bool newLayer) {
    Image img = projectVoxels(doc_.scene.voxels, view, doc_.width, doc_.height);
    if (newLayer) {
        int at = layer_ + 1, f = frame_;
        editStructure("Proyectar a sprite", ChangeLayers | ChangePixels, [&](Document& d) {
            Layer& l = d.insertRasterLayer(at, std::string("Vista ") + toString(view));
            l.cels[f] = std::make_shared<Image>(img);
        });
        setCurrentLayer(at);
        return;
    }
    const Layer& L = doc_.layers.at(layer_);
    if (L.locked || L.type != LayerType::Raster2D) return;
    PixelStroke s(doc_, layer_, frame_);
    for (int y = 0; y < img.height(); ++y)
        for (int x = 0; x < img.width(); ++x)
            if (alpha(img.pixel(x, y))) s.target().setPixel(x, y, img.pixel(x, y));
    s.markDirty(img.rect());
    commit(s.finish("Proyectar a sprite"));
}

bool Editor::generateFromFrames(int frontFrame, int sideFrame, int topFrame, bool replace) {
    auto frameImage = [&](int f) -> std::optional<Image> {
        if (f < 0 || f >= doc_.frameCount()) return std::nullopt;
        return composeFrame(doc_, f);
    };
    auto front = frameImage(frontFrame);
    if (!front || front->isFullyTransparent()) return false;
    auto side = frameImage(sideFrame);
    auto top = frameImage(topFrame);
    ViewSet vs{&*front, side ? &*side : nullptr, top ? &*top : nullptr};
    VoxelGrid g = generateFromViews(vs);
    if (g.empty()) return false;
    VoxelStroke s(doc_);
    if (replace) {
        std::vector<IVec3> cells;
        doc_.scene.voxels.forEach([&](IVec3 p, const Voxel&) { cells.push_back(p); });
        for (const IVec3& p : cells) s.set(p, Voxel{});
    }
    g.forEach([&](IVec3 p, const Voxel& v) { s.set(p, v); });
    commit(s.finish("Generar desde vistas"));
    return true;
}

// ---------------------------------------------------------------------------
// Reproduccion
// ---------------------------------------------------------------------------
void Editor::setPlaying(bool on) {
    if (on == playing_) return;
    playing_ = on;
    if (on) {
        cancelStroke2D();
        const AnimationTag* tag = nullptr;
        for (const auto& t : doc_.timeline.tags)
            if (frame_ >= t.from && frame_ <= t.to) { tag = &t; break; }
        if (tag) player_.setTag(*tag);
        else player_.setWholeTimeline(doc_.timeline);
        player_.reset();
        frame_ = player_.frame();
    }
    notify(ChangeSelection);
}

void Editor::tick(double ms) {
    if (!playing_) return;
    if (player_.advance(doc_.timeline, ms)) {
        frame_ = std::clamp(player_.frame(), 0, doc_.frameCount() - 1);
        notify(ChangeSelection);
    }
}

// ---------------------------------------------------------------------------
// Notificaciones
// ---------------------------------------------------------------------------
int Editor::subscribe(Listener l) {
    int id = nextListener_++;
    listeners_[id] = std::move(l);
    return id;
}
void Editor::unsubscribe(int id) { listeners_.erase(id); }
void Editor::notify(unsigned flags) {
    auto copy = listeners_;
    for (auto& [id, l] : copy) l(flags);
}

// ---------------------------------------------------------------------------
// Acciones con nombre
// ---------------------------------------------------------------------------
void Editor::registerActions() {
    auto& a = actions_;
    a.add({"edit.undo", "Deshacer", "Ctrl+Z", "undo"}, [this] { undo(); }, [this] { return history_.canUndo(); });
    a.add({"edit.redo", "Rehacer", "Ctrl+Shift+Z", "redo"}, [this] { redo(); }, [this] { return history_.canRedo(); });

    auto tool2 = [&](Tool2D t, const char* label, const char* key, const char* icon) {
        a.add({toolId(t), label, key, icon, true}, [this, t] { setTool2D(t); }, {}, [this, t] { return tool2d_ == t; });
    };
    tool2(Tool2D::Pencil, "Lapiz", "B", "pencil");
    tool2(Tool2D::Eraser, "Borrador", "E", "eraser");
    tool2(Tool2D::Line, "Linea", "L", "line");
    tool2(Tool2D::Rectangle, "Rectangulo", "U", "rect");
    tool2(Tool2D::FilledRectangle, "Rectangulo relleno", "Shift+U", "rect_filled");
    tool2(Tool2D::Ellipse, "Elipse", "C", "ellipse");
    tool2(Tool2D::Fill, "Bote de pintura", "G", "fill");
    tool2(Tool2D::Picker, "Cuentagotas", "I", "picker");

    auto tool3 = [&](Tool3D t, const char* label, const char* key, const char* icon) {
        a.add({toolId(t), label, key, icon, true}, [this, t] { setTool3D(t); }, {}, [this, t] { return tool3d_ == t; });
    };
    tool3(Tool3D::PlaceVoxel, "Colocar voxel", "V", "cube_add");
    tool3(Tool3D::EraseVoxel, "Borrar voxel", "Shift+V", "cube_remove");
    tool3(Tool3D::PaintVoxel, "Pintar voxel", "P", "cube_paint");
    tool3(Tool3D::DrawOnPlane, "Dibujar en plano", "D", "plane_draw");
    tool3(Tool3D::EraseOnPlane, "Borrar en plano", "Shift+D", "plane_erase");
    tool3(Tool3D::PickColor, "Cuentagotas 3D", "Alt+I", "picker");

    a.add({"brush.bigger", "Pincel mas grande", "]"}, [this] { setBrushSize(brush_.size + 1); });
    a.add({"brush.smaller", "Pincel mas pequeno", "["}, [this] { setBrushSize(brush_.size - 1); });
    a.add({"brush.pixel_perfect", "Pixel perfect", "", "", true}, [this] { setPixelPerfect(!pixelPerfect_); }, {},
          [this] { return pixelPerfect_; });
    a.add({"color.swap", "Intercambiar colores", "X", "swap"}, [this] { swapColors(); });

    a.add({"frame.next", "Frame siguiente", "."}, [this] { setCurrentFrame((frame_ + 1) % doc_.frameCount()); });
    a.add({"frame.prev", "Frame anterior", ","},
          [this] { setCurrentFrame((frame_ - 1 + doc_.frameCount()) % doc_.frameCount()); });
    a.add({"frame.add", "Nuevo frame", "Alt+N", "frame_add"}, [this] { addFrame(); });
    a.add({"frame.duplicate", "Duplicar frame", "Alt+D", "frame_dup"}, [this] { duplicateFrame(); });
    a.add({"frame.delete", "Eliminar frame", "Alt+Del", "frame_del"}, [this] { removeFrame(); },
          [this] { return doc_.frameCount() > 1; });

    a.add({"layer.add", "Nueva capa", "Shift+N", "layer_add"}, [this] { addLayer(); });
    a.add({"layer.duplicate", "Duplicar capa", "Ctrl+Alt+D", "layer_dup"}, [this] { duplicateLayer(); });
    a.add({"layer.delete", "Eliminar capa", "Shift+Del", "layer_del"}, [this] { removeLayer(); },
          [this] { return doc_.layers.size() > 1; });
    a.add({"layer.up", "Subir capa", "Ctrl+Up", "up"}, [this] { moveLayer(+1); });
    a.add({"layer.down", "Bajar capa", "Ctrl+Down", "down"}, [this] { moveLayer(-1); });

    a.add({"onion.toggle", "Onion skin", "F3", "onion", true},
          [this] {
              editStructure("Onion skin", ChangeSettings, [](Document& d) { d.onion.enabled = !d.onion.enabled; });
          },
          {}, [this] { return doc_.onion.enabled; });
    a.add({"anim.play", "Reproducir", "Return", "play", true}, [this] { setPlaying(!playing_); }, {},
          [this] { return playing_; });

    a.add({"plane.next", "Plano siguiente", "PgDown", "plane_next"}, [this] { cycleWorkplane(+1); });
    a.add({"plane.prev", "Plano anterior", "PgUp", "plane_prev"}, [this] { cycleWorkplane(-1); });
    a.add({"plane.forward", "Mover plano +1", "Alt+Up", "up"}, [this] { nudgeWorkplane(+1); });
    a.add({"plane.back", "Mover plano -1", "Alt+Down", "down"}, [this] { nudgeWorkplane(-1); });
    a.add({"plane.add_front", "Nuevo plano frontal", "", "plane_front"}, [this] { addWorkplane(PlaneAxis::Front); });
    a.add({"plane.add_side", "Nuevo plano lateral", "", "plane_side"}, [this] { addWorkplane(PlaneAxis::Side); });
    a.add({"plane.add_top", "Nuevo plano superior", "", "plane_top"}, [this] { addWorkplane(PlaneAxis::Top); });
    a.add({"plane.delete", "Eliminar plano", "", "plane_del"}, [this] { removeWorkplane(); },
          [this] { return !doc_.scene.workplanes.empty(); });
    a.add({"plane.extrude", "Extruir plano a voxeles", "Ctrl+E", "extrude"}, [this] { extrudeActivePlane(); },
          [this] { return doc_.scene.activePlane() != nullptr; });
    a.add({"voxels.clear", "Vaciar voxeles", "", "trash"}, [this] { clearVoxels(); },
          [this] { return !doc_.scene.voxels.empty(); });

    auto view = [&](const char* id, const char* label, const char* key, ViewPreset p) {
        a.add({id, label, key}, [this, p] {
            applyViewPreset(doc_.scene.camera, p);
            notify(ChangeScene3D);
        });
    };
    view("view.front", "Vista frontal", "1", ViewPreset::Front);
    view("view.back", "Vista trasera", "Ctrl+1", ViewPreset::Back);
    view("view.right", "Vista derecha", "3", ViewPreset::Right);
    view("view.left", "Vista izquierda", "Ctrl+3", ViewPreset::Left);
    view("view.top", "Vista superior", "7", ViewPreset::Top);
    view("view.iso", "Vista isometrica", "9", ViewPreset::Iso);
    a.add({"view.toggle_ortho", "Perspectiva / ortografica", "5", "", true},
          [this] {
              doc_.scene.camera.ortho = !doc_.scene.camera.ortho;
              notify(ChangeScene3D);
          },
          {}, [this] { return doc_.scene.camera.ortho; });
    a.add({"project.front", "Proyectar frente a nueva capa", "", "project"},
          [this] { projectToCel(OrthoView::Front, true); });
}

}  // namespace xeldi
