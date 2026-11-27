#include "app/Viewport3D.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>

#include "app/QtBridge.hpp"
#include "app/Theme.hpp"
#include "engine3d/CameraMath.hpp"

namespace xeldi::ui {

Viewport3D::Viewport3D(Editor& ed, QWidget* parent) : QWidget(parent), ed_(ed) {
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMinimumSize(200, 200);
    setCursor(Qt::CrossCursor);
    sub_ = ed_.subscribe([this](unsigned f) {
        if (f & (ChangeScene3D | ChangeDocument | ChangeHistory | ChangeSelection)) update();
    });
}

Viewport3D::~Viewport3D() { ed_.unsubscribe(sub_); }

void Viewport3D::setPixelScale(int s) {
    scale_ = std::clamp(s, 1, 4);
    update();
}

PointerInput Viewport3D::toRender(QPointF p, bool primary) const {
    return PointerInput{float(p.x() / scale_), float(p.y() / scale_), 1.0f, primary};
}

void Viewport3D::paintEvent(QPaintEvent*) {
    const auto& d = ed_.doc();
    RenderOptions opt;
    opt.hoverCell = hover_;
    opt.gridHalfExtent = std::max(16, std::max(d.width, d.height) / 2);
    const Image& img = renderer_.render(d.scene, d.scene.camera, rw(), rh(), opt);
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.drawImage(QRect(0, 0, rw() * scale_, rh() * scale_), toQImage(img));
    if (rw() * scale_ < width() || rh() * scale_ < height()) {
        p.fillRect(QRect(rw() * scale_, 0, width(), height()), theme::kWorkspace);
        p.fillRect(QRect(0, rh() * scale_, width(), height()), theme::kWorkspace);
    }

    // Etiqueta de vista + plano activo (estilo ventanita retro).
    const auto& cam = d.scene.camera;
    QString view = cam.ortho ? tr("Ortográfica") : tr("Perspectiva");
    const Workplane* pl = d.scene.activeWorkplane < int(d.scene.workplanes.size())
                              ? &d.scene.workplanes[d.scene.activeWorkplane] : nullptr;
    QString text = view + "  ·  " + (pl ? qs(pl->name) : tr("sin plano"));
    QFontMetrics fm(font());
    QRect box(10, 10, fm.horizontalAdvance(text) + 20, fm.height() + 10);
    p.setPen(QPen(theme::kPlum, 2));
    p.setBrush(QColor(0xFF, 0xF8, 0xFA, 230));
    p.drawRoundedRect(box, 8, 8);
    p.setPen(theme::kText);
    p.drawText(box, Qt::AlignCenter, text);

    QString help = tr("Rueda: zoom · Botón central/derecho: orbitar · Shift: desplazar");
    p.setPen(QColor(0x8C, 0x74, 0x80));
    p.drawText(QRect(10, height() - fm.height() - 8, width() - 20, fm.height() + 4), Qt::AlignLeft, help);
    p.setPen(QPen(theme::kPlum, 2));
    p.setBrush(Qt::NoBrush);
    p.drawRect(rect().adjusted(1, 1, -1, -1));
}

void Viewport3D::updateHover(QPointF pos) {
    auto h = ed_.hoverCell(float(pos.x() / scale_), float(pos.y() / scale_), rw(), rh());
    if (h != hover_) {
        hover_ = h;
        if (h) emit statusText(tr("Celda %1, %2, %3").arg(h->x).arg(h->y).arg(h->z));
        else emit statusText(QString());
        update();
    }
}

void Viewport3D::mousePressEvent(QMouseEvent* e) {
    setFocus();
    lastMouse_ = e->position().toPoint();
    bool alt = e->modifiers() & Qt::AltModifier;
    bool shift = e->modifiers() & Qt::ShiftModifier;
    if (e->button() == Qt::MiddleButton || e->button() == Qt::RightButton || (e->button() == Qt::LeftButton && alt)) {
        drag_ = shift ? Drag::Pan : Drag::Orbit;
        setCursor(drag_ == Drag::Pan ? Qt::ClosedHandCursor : Qt::SizeAllCursor);
        return;
    }
    if (e->button() == Qt::LeftButton) {
        drag_ = Drag::Tool;
        ed_.beginStroke3D(toRender(e->position(), !(e->modifiers() & Qt::ControlModifier)), rw(), rh());
    }
}

void Viewport3D::mouseMoveEvent(QMouseEvent* e) {
    QPoint pos = e->position().toPoint();
    QPoint delta = pos - lastMouse_;
    lastMouse_ = pos;
    auto& cam = ed_.camera();
    switch (drag_) {
        case Drag::Orbit:
            orbitCamera(cam, -delta.x() * 0.5f, delta.y() * 0.5f);
            ed_.cameraChanged();
            return;
        case Drag::Pan:
            panCamera(cam, float(delta.x()), float(delta.y()), height());
            ed_.cameraChanged();
            return;
        case Drag::Tool:
            ed_.moveStroke3D(toRender(e->position(), !(e->modifiers() & Qt::ControlModifier)), rw(), rh());
            updateHover(e->position());
            return;
        case Drag::None:
            updateHover(e->position());
            return;
    }
}

void Viewport3D::mouseReleaseEvent(QMouseEvent*) {
    if (drag_ == Drag::Tool) ed_.endStroke3D();
    drag_ = Drag::None;
    setCursor(Qt::CrossCursor);
}

void Viewport3D::wheelEvent(QWheelEvent* e) {
    float steps = e->angleDelta().y() / 120.0f;
    zoomCamera(ed_.camera(), std::pow(0.88f, steps));
    ed_.cameraChanged();
}

void Viewport3D::leaveEvent(QEvent*) {
    hover_.reset();
    update();
}

}  // namespace xeldi::ui
