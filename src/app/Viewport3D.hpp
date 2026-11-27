#pragma once
// Viewport 3D: navegacion estilo Blender y herramientas de voxel/plano.
// Usa el renderizador por software del motor (funciona en X11, Wayland y sin GPU).
#include <QImage>
#include <QWidget>
#include <optional>

#include "editor/Editor.hpp"
#include "engine3d/SoftwareRenderer.hpp"

namespace xeldi::ui {

class Viewport3D : public QWidget {
    Q_OBJECT
public:
    explicit Viewport3D(Editor& ed, QWidget* parent = nullptr);
    ~Viewport3D() override;

    /// 1 = resolucion completa, 2 = mitad (pixeles gorditos, mas rapido), 3 = un tercio.
    void setPixelScale(int s);
    int pixelScale() const { return scale_; }

signals:
    void statusText(const QString& text);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    int rw() const { return std::max(1, width() / scale_); }
    int rh() const { return std::max(1, height() / scale_); }
    PointerInput toRender(QPointF p, bool primary) const;
    void updateHover(QPointF p);

    Editor& ed_;
    int sub_ = 0;
    int scale_ = 2;
    SoftwareRenderer renderer_;
    enum class Drag { None, Tool, Orbit, Pan } drag_ = Drag::None;
    QPoint lastMouse_;
    std::optional<IVec3> hover_;
};

}  // namespace xeldi::ui
