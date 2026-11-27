#pragma once
// Renderizador 3D por software (rasterizador con z-buffer), sin Qt ni OpenGL.
//  * Funciona igual en X11, Wayland, maquinas sin GPU y en pruebas automaticas.
//  * El mismo codigo sirve para generar miniaturas y vistas para sprites.
// Un backend OpenGL/Vulkan podra reemplazarlo en el viewport sin tocar el modelo.
#include <optional>
#include <vector>

#include "core/Document.hpp"
#include "engine3d/CameraMath.hpp"

namespace xeldi {

struct RenderOptions {
    Rgba background = 0xF7E8EEFFu;
    bool shading = true;
    bool showGrid = true;
    int gridHalfExtent = 32;
    Rgba gridColor = 0xE3C9D3FFu;
    Rgba gridMajorColor = 0xD2A9BBFFu;
    bool showAxes = true;
    bool showWorkplanes = true;
    float inactivePlaneOpacity = 0.35f;
    Rgba planeFrameColor = 0xA775C9FFu;
    Rgba activePlaneFrameColor = 0xE86A92FFu;
    std::optional<IVec3> hoverCell;  ///< celda resaltada (cubo de alambre)
    Rgba hoverColor = 0x5A3D52FFu;
};

class SoftwareRenderer {
public:
    /// Renderiza la escena con la camara dada a una imagen w x h.
    const Image& render(const Scene3D& scene, const CameraState& cam, int w, int h, const RenderOptions& opt = {});
    const Image& image() const { return color_; }
    const CameraMatrices& matrices() const { return m_; }

    /// Primitivas publicas (se usan para overlays y pruebas).
    void drawLine3D(Vec3 a, Vec3 b, Rgba color, bool depthTest = true);
    void fillQuad(const Vec3 q[4], Rgba color, float opacity = 1.0f, bool writeDepth = true);

private:
    void fillTriangle(const Vec3 s[3], Rgba color, float opacity, bool writeDepth);
    bool toScreen(Vec3 p, Vec3& s) const;
    void drawVoxels(const VoxelGrid& grid, bool shading);
    void drawWorkplane(const Workplane& p, bool active, const RenderOptions& opt);
    void drawGrid(const RenderOptions& opt);
    void drawWireCube(IVec3 c, Rgba color);

    Image color_;
    std::vector<float> depth_;
    CameraMatrices m_;
    int w_ = 0, h_ = 0;
};

}  // namespace xeldi
