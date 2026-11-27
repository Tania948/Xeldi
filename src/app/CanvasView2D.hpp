#pragma once
// Lienzo 2D: zoom/pan, cuadricula de pixeles, onion skin y herramientas.
// Solo traduce eventos de raton/lapiz/gestos a llamadas del Editor.
#include <QImage>
#include <QWidget>

#include "editor/Editor.hpp"

class QGestureEvent;

namespace xeldi::ui {

class CanvasView2D : public QWidget {
    Q_OBJECT
public:
    explicit CanvasView2D(Editor& ed, QWidget* parent = nullptr);
    ~CanvasView2D() override;

    void fitToView();
    void setZoom(float z, QPointF anchor);
    float zoom() const { return zoom_; }
    void setShowGrid(bool on) { showGrid_ = on; update(); }
    bool showGrid() const { return showGrid_; }

signals:
    void hoverPixelChanged(int x, int y, bool inside);
    void zoomChanged(float zoom);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void keyReleaseEvent(QKeyEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void leaveEvent(QEvent*) override;
    bool event(QEvent*) override;

private:
    QPointF toCanvas(QPointF widgetPos) const;
    void rebuildCache();
    bool gestureEvent(QGestureEvent* e);

    Editor& ed_;
    int sub_ = 0;
    float zoom_ = 8.0f;
    QPointF origin_{20, 20};  ///< posicion en el widget de la esquina superior izquierda
    bool panning_ = false, spaceHeld_ = false, needsFit_ = true, showGrid_ = true;
    bool cacheDirty_ = true;
    QPoint lastMouse_;
    QPoint hover_{-1, -1};
    QImage frameCache_, onionCache_;
};

}  // namespace xeldi::ui
