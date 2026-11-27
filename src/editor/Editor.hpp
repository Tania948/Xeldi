#pragma once
// Editor: estado de edicion + historial + acciones con nombre.
// Es la "fachada" que usa cualquier frontend (Qt de escritorio hoy, tactil
// manana). No depende de Qt: la UI solo llama metodos y escucha cambios.
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "core/Actions.hpp"
#include "core/Commands.hpp"
#include "core/Document.hpp"
#include "engine2d/AnimationPlayer.hpp"
#include "engine2d/BrushTools.hpp"
#include "engine3d/CameraMath.hpp"
#include "engine3d/ViewProjection.hpp"

namespace xeldi {

enum class Tool2D { Pencil, Eraser, Line, Rectangle, FilledRectangle, Ellipse, Fill, Picker };
enum class Tool3D { PlaceVoxel, EraseVoxel, PaintVoxel, DrawOnPlane, EraseOnPlane, PickColor };

const char* toolId(Tool2D t);
const char* toolId(Tool3D t);

/// Entrada de puntero independiente del dispositivo (raton, lapiz, dedo).
struct PointerInput {
    float x = 0, y = 0;       ///< coordenadas en el espacio del lienzo/viewport
    float pressure = 1.0f;    ///< 0..1 (lapiz)
    bool primary = true;      ///< false = boton/gesto secundario (borrar con color secundario)
};

class Editor {
public:
    Editor();

    // --- Documento ---------------------------------------------------------
    Document& doc() { return doc_; }
    const Document& doc() const { return doc_; }
    void setDocument(Document d, std::string path = {});
    const std::string& filePath() const { return path_; }
    void setFilePath(std::string p) { path_ = std::move(p); }
    bool isModified() const { return !history_.isClean(); }
    void markSaved() { history_.markClean(); notify(ChangeHistory); }

    // --- Historial -----------------------------------------------------------
    History& history() { return history_; }
    /// Registra un comando ya aplicado.
    void commit(std::unique_ptr<Command> cmd);
    bool undo();
    bool redo();
    /// Cambio estructural (capas/frames/tags/ajustes) como un paso de deshacer.
    void editStructure(const std::string& label, unsigned flags, const std::function<void(Document&)>& fn);
    /// Cambio de planos de trabajo como un paso de deshacer.
    void editSceneLayout(const std::string& label, const std::function<void(Document&)>& fn);

    // --- Estado de edicion -----------------------------------------------------
    int currentFrame() const { return frame_; }
    int currentLayer() const { return layer_; }
    void setCurrentFrame(int f);
    void setCurrentLayer(int l);
    Rgba primaryColor() const { return primary_; }
    Rgba secondaryColor() const { return secondary_; }
    void setPrimaryColor(Rgba c);
    void setSecondaryColor(Rgba c);
    void swapColors();
    Tool2D tool2D() const { return tool2d_; }
    Tool3D tool3D() const { return tool3d_; }
    void setTool2D(Tool2D t);
    void setTool3D(Tool3D t);
    int brushSize() const { return brush_.size; }
    void setBrushSize(int s);
    bool pixelPerfect() const { return pixelPerfect_; }
    void setPixelPerfect(bool on) { pixelPerfect_ = on; notify(ChangeSelection); }
    int extrudeDepth() const { return extrudeDepth_; }
    void setExtrudeDepth(int d) { extrudeDepth_ = d; notify(ChangeSelection); }

    // --- Trazos 2D (coordenadas de pixel del lienzo) --------------------------
    void beginStroke2D(const PointerInput& p);
    void moveStroke2D(const PointerInput& p);
    void endStroke2D(const PointerInput& p);
    bool strokeActive2D() const { return bool(stroke2d_); }
    void cancelStroke2D();
    /// Color compuesto del frame actual bajo el cursor (cuentagotas).
    Rgba sampleColor(int x, int y) const;

    // --- Edicion 3D (coordenadas de pantalla del viewport) --------------------
    void beginStroke3D(const PointerInput& p, int vw, int vh);
    void moveStroke3D(const PointerInput& p, int vw, int vh);
    void endStroke3D();
    /// Celda que se resaltaria bajo el cursor para la herramienta actual.
    std::optional<IVec3> hoverCell(float sx, float sy, int vw, int vh) const;

    // --- Operaciones de alto nivel (todas deshacibles) -------------------------
    void addLayer();
    void duplicateLayer();
    void removeLayer();
    void moveLayer(int delta);
    void addFrame();
    void duplicateFrame();
    void removeFrame();
    void addTagFromSelection(const std::string& name, int from, int to);
    void addWorkplane(PlaneAxis axis);
    void removeWorkplane();
    void nudgeWorkplane(int steps);
    void cycleWorkplane(int delta);
    void extrudeActivePlane();
    void clearVoxels();
    /// Proyecta los voxeles a la cel actual (como nueva capa si `newLayer`).
    void projectToCel(OrthoView view, bool newLayer);
    /// Genera voxeles desde 3 frames de la capa actual: frente, lado y arriba (-1 = sin vista).
    bool generateFromFrames(int frontFrame, int sideFrame, int topFrame, bool replace);

    // --- Reproduccion en el lienzo principal -----------------------------------
    bool isPlaying() const { return playing_; }
    void setPlaying(bool on);
    /// La UI llama esto con el tiempo transcurrido (ms).
    void tick(double ms);

    // --- Camara ---------------------------------------------------------------
    CameraState& camera() { return doc_.scene.camera; }
    void cameraChanged() { notify(ChangeScene3D); }

    // --- Notificaciones -----------------------------------------------------------
    using Listener = std::function<void(unsigned flags)>;
    int subscribe(Listener l);
    void unsubscribe(int id);
    void notify(unsigned flags);

    ActionRegistry& actions() { return actions_; }

private:
    void registerActions();
    void clampSelection();
    void applyTool2D(Point a, Point b, bool preview);
    Ray rayFor(const PointerInput& p, int vw, int vh) const;
    void applyTool3D(const PointerInput& p, int vw, int vh);

    Document doc_;
    History history_;
    ActionRegistry actions_;
    std::string path_;
    int frame_ = 0, layer_ = 0;
    Rgba primary_ = 0x5A3D52FFu, secondary_ = 0xF7D6E0FFu;
    Tool2D tool2d_ = Tool2D::Pencil;
    Tool3D tool3d_ = Tool3D::PlaceVoxel;
    BrushOptions brush_;
    bool pixelPerfect_ = true;
    int extrudeDepth_ = 1;

    std::unique_ptr<PixelStroke> stroke2d_;
    Point strokeStart_, strokeLast_;
    std::vector<Point> strokePath_;
    Rgba strokeColor_ = 0;

    std::unique_ptr<VoxelStroke> voxelStroke_;
    std::unique_ptr<PlaneStroke> planeStroke_;
    std::optional<IVec3> lastCell3d_;
    std::optional<std::pair<int, int>> lastPlanePx_;
    bool stroke3dPrimary_ = true;

    bool playing_ = false;
    AnimationPlayer player_;

    std::map<int, Listener> listeners_;
    int nextListener_ = 1;
};

}  // namespace xeldi
