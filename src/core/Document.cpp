#include "core/Document.hpp"

#include <algorithm>
#include <cmath>
#include <set>

namespace xeldi {

const char* toString(LayerType t) { return t == LayerType::Spatial3D ? "spatial3d" : "raster2d"; }
LayerType layerTypeFromString(const std::string& s) {
    return s == "spatial3d" ? LayerType::Spatial3D : LayerType::Raster2D;
}

const char* toString(BlendMode b) {
    switch (b) {
        case BlendMode::Normal: return "normal";
        case BlendMode::Multiply: return "multiply";
        case BlendMode::Screen: return "screen";
        case BlendMode::Add: return "add";
        case BlendMode::Darken: return "darken";
        case BlendMode::Lighten: return "lighten";
        case BlendMode::Overlay: return "overlay";
    }
    return "normal";
}
BlendMode blendModeFromString(const std::string& s) {
    if (s == "multiply") return BlendMode::Multiply;
    if (s == "screen") return BlendMode::Screen;
    if (s == "add") return BlendMode::Add;
    if (s == "darken") return BlendMode::Darken;
    if (s == "lighten") return BlendMode::Lighten;
    if (s == "overlay") return BlendMode::Overlay;
    return BlendMode::Normal;
}

Vec3 CameraState::position() const {
    float yaw = radians(yawDeg), pitch = radians(pitchDeg);
    Vec3 dir{std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw)};
    return target + dir * distance;
}

Workplane* Scene3D::findPlane(const std::string& id) {
    for (auto& w : workplanes)
        if (w.id == id) return &w;
    return nullptr;
}

Document Document::createBlank(int w, int h) {
    Document d;
    d.width = w;
    d.height = h;
    d.timeline.frames = {Frame{}};
    d.insertRasterLayer(0, "Capa 1");
    d.scene.workplanes.push_back(
        Workplane::axisAligned(PlaneAxis::Front, 0, std::max(w, h) / 2 * 2, "plane_front", "Plano frontal"));
    d.scene.camera.target = {0, float(h) / 4.0f, 0};
    d.scene.camera.distance = float(std::max(w, h)) * 1.4f;
    return d;
}

int Document::layerIndex(const std::string& id) const {
    for (int i = 0; i < int(layers.size()); ++i)
        if (layers[i].id == id) return i;
    return -1;
}

Layer* Document::findLayer(const std::string& id) {
    int i = layerIndex(id);
    return i >= 0 ? &layers[i] : nullptr;
}

Image* Document::cel(int l, int f) {
    if (l < 0 || l >= int(layers.size()) || f < 0 || f >= int(layers[l].cels.size())) return nullptr;
    return layers[l].cels[f].get();
}
const Image* Document::cel(int l, int f) const { return const_cast<Document*>(this)->cel(l, f); }

Image& Document::ensureCel(int l, int f) {
    auto& slot = layers.at(l).cels.at(f);
    if (!slot) slot = std::make_shared<Image>(width, height);
    return *slot;
}

std::string Document::newId(const std::string& prefix) const {
    std::set<std::string> used;
    for (const auto& l : layers) used.insert(l.id);
    for (const auto& w : scene.workplanes) used.insert(w.id);
    for (int i = 1;; ++i) {
        std::string id = prefix + "_" + std::to_string(i);
        if (!used.count(id)) return id;
    }
}

Layer& Document::insertRasterLayer(int at, const std::string& lname) {
    Layer l;
    l.id = newId("layer");
    l.name = lname;
    l.cels.resize(std::size_t(frameCount()));
    at = std::clamp(at, 0, int(layers.size()));
    return *layers.insert(layers.begin() + at, std::move(l));
}

void Document::insertFrame(int at, Frame f) {
    at = std::clamp(at, 0, frameCount());
    timeline.frames.insert(timeline.frames.begin() + at, f);
    for (auto& l : layers) l.cels.insert(l.cels.begin() + at, nullptr);
    timeline.onFrameInserted(at);
}

void Document::removeFrame(int at) {
    if (frameCount() <= 1 || at < 0 || at >= frameCount()) return;
    timeline.frames.erase(timeline.frames.begin() + at);
    for (auto& l : layers) l.cels.erase(l.cels.begin() + at);
    timeline.onFrameRemoved(at);
}

void Document::duplicateFrame(int at) {
    if (at < 0 || at >= frameCount()) return;
    insertFrame(at + 1, timeline.frames[at]);
    for (auto& l : layers)
        if (l.cels[at]) l.cels[at + 1] = std::make_shared<Image>(*l.cels[at]);
}

bool Document::validate(std::string* error) const {
    auto fail = [&](const std::string& m) {
        if (error) *error = m;
        return false;
    };
    if (width <= 0 || height <= 0) return fail("tamano de lienzo invalido");
    if (frameCount() < 1) return fail("el documento necesita al menos un frame");
    std::set<std::string> ids;
    for (const auto& l : layers) {
        if (!ids.insert(l.id).second) return fail("id de capa duplicado: " + l.id);
        if (int(l.cels.size()) != frameCount()) return fail("capa " + l.id + " con numero de cels incorrecto");
        for (const auto& c : l.cels)
            if (c && (c->width() != width || c->height() != height)) return fail("cel con tamano incorrecto");
    }
    for (const auto& t : timeline.tags)
        if (t.from < 0 || t.to >= frameCount() || t.from > t.to) return fail("tag fuera de rango: " + t.name);
    return true;
}

}  // namespace xeldi
