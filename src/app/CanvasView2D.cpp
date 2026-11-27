#include "app/CanvasView2D.hpp"

#include <QGestureEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPinchGesture>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

#include "app/QtBridge.hpp"
#include "app/Theme.hpp"
#include "engine2d/Compositor.hpp"
#include "engine2d/OnionSkin.hpp"

namespace xeldi::ui {

CanvasView2D::CanvasView2D(Editor& ed, QWidget* parent) : QWidget(parent), ed_(ed) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_AcceptTouchEvents);
    grabGesture(Qt::PinchGesture);
    setMinimumSize(200, 200);
    setCursor(Qt::CrossCursor);
    sub_ = ed_.subscribe([this](unsigned f) {
        if (f & ChangeDocument) needsFit_ = true;
        if (f & (ChangePixels | ChangeLayers | ChangeFrames | ChangeSelection | ChangeSettings | ChangeDocument |
                 ChangeHistory))
            cacheDirty_ = true;
        update();
    });
}

CanvasView2D::~CanvasView2D() { ed_.unsubscribe(sub_); }

void CanvasView2D::fitToView() {
    const auto& d = ed_.doc();
    float zx = (width() - 40.0f) / d.width, zy = (height() - 40.0f) / d.height;
    float z = std::max(1.0f, std::floor(std::min(zx, zy)));
    zoom_ = std::min(z, 64.0f);
    origin_ = QPointF((width() - d.width * zoom_) / 2.0, (height() - d.height * zoom_) / 2.0);
    needsFit_ = false;
    emit zoomChanged(zoom_);
    update();
}

void CanvasView2D::setZoom(float z, QPointF anchor) {
    z = std::clamp(z, 1.0f, 96.0f);
    QPointF c = toCanvas(anchor);
    zoom_ = z;
    origin_ = anchor - c * zoom_;
    emit zoomChanged(zoom_);
    update();
}

QPointF CanvasView2D::toCanvas(QPointF p) const { return (p - origin_) / zoom_; }

void CanvasView2D::rebuildCache() {
    const auto& d = ed_.doc();
    frameCache_ = toQImage(composeFrame(d, ed_.currentFrame()));
    onionCache_ = QImage();
    if (d.onion.enabled && !ed_.isPlaying() && d.frameCount() > 1) {
        Image onion(d.width, d.height);
        const AnimationTag* tag = nullptr;
        for (const auto& t : d.timeline.tags)
            if (ed_.currentFrame() >= t.from && ed_.currentFrame() <= t.to) { tag = &t; break; }
        renderOnion(d, ed_.currentFrame(), d.onion, ed_.currentLayer(), onion, tag);
        onionCache_ = toQImage(onion);
    }
    cacheDirty_ = false;
}

void CanvasView2D::paintEvent(QPaintEvent*) {
    if (needsFit_ && width() > 50) fitToView();
    if (cacheDirty_) rebuildCache();
    const auto& d = ed_.doc();
    QPainter p(this);
    p.fillRect(rect(), theme::kWorkspace);
    // Patron de puntitos retro en el fondo del area de trabajo.
    p.setPen(QColor(0xE0, 0xC8, 0xD2));
    for (int y = 6; y < height(); y += 12)
        for (int x = (y / 12) % 2 ? 12 : 6; x < width(); x += 12) p.drawPoint(x, y);

    QRectF cr(origin_.x(), origin_.y(), d.width * zoom_, d.height * zoom_);
    // Sombra + marco
    p.fillRect(cr.translated(5, 5), QColor(0x5A, 0x3D, 0x52, 60));
    // Cuadros de transparencia (cada 8 pixeles de documento, minimo 8 px en pantalla)
    float cell = std::max(8.0f, zoom_ * 4.0f);
    p.save();
    p.setClipRect(cr);
    for (float y = 0; y < cr.height(); y += cell)
        for (float x = 0; x < cr.width(); x += cell) {
            bool odd = (int(x / cell) + int(y / cell)) % 2;
            p.fillRect(QRectF(cr.x() + x, cr.y() + y, cell, cell), odd ? theme::kCheckB : theme::kCheckA);
        }
    p.restore();
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    if (!onionCache_.isNull()) p.drawImage(cr, onionCache_);
    p.drawImage(cr, frameCache_);

    if (showGrid_ && zoom_ >= 8) {
        p.setPen(QColor(0x5A, 0x3D, 0x52, 28));
        for (int x = 1; x < d.width; ++x) p.drawLine(QPointF(cr.x() + x * zoom_, cr.top()), QPointF(cr.x() + x * zoom_, cr.bottom()));
        for (int y = 1; y < d.height; ++y) p.drawLine(QPointF(cr.left(), cr.y() + y * zoom_), QPointF(cr.right(), cr.y() + y * zoom_));
    }
    p.setPen(QPen(theme::kPlum, 2));
    p.drawRect(cr.adjusted(-1, -1, 1, 1));

    // Cursor del pincel
    if (hover_.x() >= 0 && !panning_) {
        int s = (ed_.tool2D() == Tool2D::Fill || ed_.tool2D() == Tool2D::Picker) ? 1 : ed_.brushSize();
        int x0 = hover_.x() - (s - 1) / 2, y0 = hover_.y() - (s - 1) / 2;
        QRectF br(origin_.x() + x0 * zoom_, origin_.y() + y0 * zoom_, s * zoom_, s * zoom_);
        p.setPen(QPen(QColor(255, 255, 255, 200), 1));
        p.drawRect(br.adjusted(-1, -1, 1, 1));
        p.setPen(QPen(theme::kPinkStrong, 1));
        p.drawRect(br);
    }
    // Indicador de reproduccion
    if (ed_.isPlaying()) {
        p.setPen(Qt::NoPen);
        p.setBrush(theme::kPinkStrong);
        p.drawRoundedRect(QRectF(10, 10, 74, 22), 8, 8);
        p.setPen(Qt::white);
        p.drawText(QRectF(10, 10, 74, 22), Qt::AlignCenter, tr("▶ play"));
    }
}

static PointerInput pointer(QPointF c, bool primary) { return PointerInput{float(c.x()), float(c.y()), 1.0f, primary}; }

void CanvasView2D::mousePressEvent(QMouseEvent* e) {
    setFocus();
    lastMouse_ = e->position().toPoint();
    if (e->button() == Qt::MiddleButton || (e->button() == Qt::LeftButton && spaceHeld_)) {
        panning_ = true;
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (ed_.isPlaying()) ed_.setPlaying(false);
    if (e->button() == Qt::LeftButton || e->button() == Qt::RightButton)
        ed_.beginStroke2D(pointer(toCanvas(e->position()), e->button() == Qt::LeftButton));
}

void CanvasView2D::mouseMoveEvent(QMouseEvent* e) {
    QPoint pos = e->position().toPoint();
    if (panning_) {
        origin_ += pos - lastMouse_;
        lastMouse_ = pos;
        update();
        return;
    }
    QPointF c = toCanvas(e->position());
    QPoint h(int(std::floor(c.x())), int(std::floor(c.y())));
    if (h != hover_) {
        hover_ = h;
        bool inside = h.x() >= 0 && h.y() >= 0 && h.x() < ed_.doc().width && h.y() < ed_.doc().height;
        emit hoverPixelChanged(h.x(), h.y(), inside);
        update();
    }
    if (ed_.strokeActive2D()) ed_.moveStroke2D(pointer(c, !(e->buttons() & Qt::RightButton)));
}

void CanvasView2D::mouseReleaseEvent(QMouseEvent* e) {
    if (panning_) {
        panning_ = false;
        setCursor(spaceHeld_ ? Qt::OpenHandCursor : Qt::CrossCursor);
        return;
    }
    if (ed_.strokeActive2D()) ed_.endStroke2D(pointer(toCanvas(e->position()), e->button() == Qt::LeftButton));
}

void CanvasView2D::wheelEvent(QWheelEvent* e) {
    float steps = e->angleDelta().y() / 120.0f;
    if (steps == 0) return;
    float z = zoom_;
    for (int i = 0; i < int(std::fabs(steps) + 0.5f); ++i) z = steps > 0 ? z * 1.25f : z / 1.25f;
    if (z >= 2) z = std::round(z);
    setZoom(z, e->position());
}

void CanvasView2D::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Space && !e->isAutoRepeat()) {
        spaceHeld_ = true;
        setCursor(Qt::OpenHandCursor);
        return;
    }
    if (e->key() == Qt::Key_Escape) ed_.cancelStroke2D();
    QWidget::keyPressEvent(e);
}

void CanvasView2D::keyReleaseEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Space && !e->isAutoRepeat()) {
        spaceHeld_ = false;
        setCursor(Qt::CrossCursor);
        return;
    }
    QWidget::keyReleaseEvent(e);
}

void CanvasView2D::resizeEvent(QResizeEvent*) {
    if (needsFit_) fitToView();
}

void CanvasView2D::leaveEvent(QEvent*) {
    hover_ = {-1, -1};
    emit hoverPixelChanged(-1, -1, false);
    update();
}

bool CanvasView2D::event(QEvent* e) {
    if (e->type() == QEvent::Gesture) return gestureEvent(static_cast<QGestureEvent*>(e));
    return QWidget::event(e);
}

// Pellizco con dos dedos = zoom (preparacion para tablets).
bool CanvasView2D::gestureEvent(QGestureEvent* e) {
    if (auto* g = static_cast<QPinchGesture*>(e->gesture(Qt::PinchGesture))) {
        if (g->changeFlags() & QPinchGesture::ScaleFactorChanged)
            setZoom(zoom_ * float(g->scaleFactor()), mapFromGlobal(g->centerPoint().toPoint()));
        if (g->changeFlags() & QPinchGesture::CenterPointChanged) {
            origin_ += g->centerPoint() - g->lastCenterPoint();
            update();
        }
    }
    return true;
}

}  // namespace xeldi::ui
