#pragma once
// Modelo de datos completo de un proyecto Xeldi. Sin Qt.
#include <string>
#include <vector>

#include "core/Camera.hpp"
#include "core/Layer.hpp"
#include "core/Palette.hpp"
#include "core/Timeline.hpp"
#include "core/VoxelGrid.hpp"
#include "core/Workplane.hpp"

namespace xeldi {

struct OnionSkinSettings {
    bool enabled = true;
    int prevFrames = 2;
    int nextFrames = 1;
    Rgba prevTint = 0xF2709BFFu;  ///< rosa fresa (pasado)
    Rgba nextTint = 0x7FA2F5FFu;  ///< azul pastel (futuro)
    float baseOpacity = 0.35f;
    float falloff = 0.65f;
    bool onlyCurrentLayer = false;
    bool loopWithinTag = true;  ///< en un tag ciclico, el anterior al primero es el ultimo
};

struct Scene3D {
    VoxelGrid voxels;
    std::vector<Workplane> workplanes;
    int activeWorkplane = 0;
    CameraState camera;

    Workplane* activePlane() {
        return (activeWorkplane >= 0 && activeWorkplane < int(workplanes.size())) ? &workplanes[activeWorkplane]
                                                                                   : nullptr;
    }
    Workplane* findPlane(const std::string& id);
};

class Document {
public:
    std::string name = "Sin titulo";
    int width = 64;
    int height = 64;
    float unitScale = 1.0f;
    Palette palette = Palette::xeldiDefault();
    Timeline timeline;
    OnionSkinSettings onion;
    std::vector<Layer> layers;  ///< indice 0 = fondo (abajo)
    Scene3D scene;

    /// Documento nuevo con una capa raster, un frame y un plano frontal.
    static Document createBlank(int w, int h);

    int frameCount() const { return timeline.frameCount(); }
    int layerIndex(const std::string& id) const;
    Layer* findLayer(const std::string& id);

    /// Cel existente (o nullptr si esta vacia / fuera de rango).
    Image* cel(int layer, int frame);
    const Image* cel(int layer, int frame) const;
    /// Cel garantizada (la crea transparente si no existe).
    Image& ensureCel(int layer, int frame);

    Layer& insertRasterLayer(int at, const std::string& name);
    void insertFrame(int at, Frame f = {});
    void removeFrame(int at);
    void duplicateFrame(int at);

    /// Comprueba invariantes (cels == frames, tamanos de imagen, ids unicos).
    bool validate(std::string* error = nullptr) const;

    std::string newId(const std::string& prefix) const;
};

}  // namespace xeldi
