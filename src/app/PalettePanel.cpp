#include "app/PalettePanel.hpp"

#include <QColorDialog>
#include <QMouseEvent>
#include <QPainter>

#include "app/QtBridge.hpp"
#include "app/Theme.hpp"

namespace xeldi::ui {

PalettePanel::PalettePanel(Editor& ed, QWidget* parent) : QWidget(parent), ed_(ed) {
    setToolTip(tr("Clic: color primario · Clic derecho: secundario · Doble clic: editar"));
    sub_ = ed_.subscribe([this](unsigned f) {
        if (f & (ChangeSelection | ChangePalette | ChangeDocument | ChangeHistory | ChangeSettings)) {
            updateGeometry();
            update();
        }
    });
    QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sp.setHeightForWidth(true);
    setSizePolicy(sp);
}

PalettePanel::~PalettePanel() { ed_.unsubscribe(sub_); }

int PalettePanel::columns() const { return std::max(1, (width() - 20) / kCell); }

int PalettePanel::heightForWidth(int w) const {
    int cols = std::max(1, (w - 20) / kCell);
    int n = int(ed_.doc().palette.colors.size());
    return kTop + ((n + cols - 1) / cols) * kCell + 12;
}

QRect PalettePanel::swatchRect(int i) const {
    int c = columns();
    return {10 + (i % c) * kCell, kTop + (i / c) * kCell, kCell - 2, kCell - 2};
}

int PalettePanel::indexAt(QPoint p) const {
    for (int i = 0; i < int(ed_.doc().palette.colors.size()); ++i)
        if (swatchRect(i).contains(p)) return i;
    return -1;
}

static void drawSwatch(QPainter& p, QRect r, QColor c, bool selected) {
    // Cuadritos para la transparencia
    p.fillRect(r, theme::kCheckA);
    p.fillRect(QRect(r.x(), r.y(), r.width() / 2, r.height() / 2), theme::kCheckB);
    p.fillRect(QRect(r.center().x(), r.center().y(), r.width() / 2 + 1, r.height() / 2 + 1), theme::kCheckB);
    p.fillRect(r, c);
    p.setPen(QPen(selected ? theme::kPinkStrong : theme::kPlum, selected ? 3 : 2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r, 4, 4);
}

void PalettePanel::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    drawSwatch(p, secondaryRect(), toQColor(ed_.secondaryColor()), false);
    drawSwatch(p, primaryRect(), toQColor(ed_.primaryColor()), false);
    p.setPen(theme::kText);
    p.drawText(QRect(74, 10, width() - 120, 20), Qt::AlignLeft | Qt::AlignVCenter, qs(toHex(ed_.primaryColor())));
    p.setPen(QColor(0x8C, 0x74, 0x80));
    p.drawText(QRect(74, 30, width() - 120, 20), Qt::AlignLeft | Qt::AlignVCenter, tr("X: intercambiar"));
    // Boton "+"
    QRect a = addRect();
    p.setPen(QPen(theme::kPlum, 2));
    p.setBrush(theme::kPanel);
    p.drawRoundedRect(a, 6, 6);
    p.setPen(theme::kText);
    p.drawText(a, Qt::AlignCenter, "+");
    const auto& cols = ed_.doc().palette.colors;
    for (int i = 0; i < int(cols.size()); ++i)
        drawSwatch(p, swatchRect(i), toQColor(cols[i]), cols[i] == ed_.primaryColor());
}

void PalettePanel::mousePressEvent(QMouseEvent* e) {
    QPoint pos = e->position().toPoint();
    if (primaryRect().contains(pos) || secondaryRect().contains(pos)) {
        bool prim = primaryRect().contains(pos);
        QColor c = QColorDialog::getColor(toQColor(prim ? ed_.primaryColor() : ed_.secondaryColor()), this,
                                          tr("Elegir color"), QColorDialog::ShowAlphaChannel);
        if (c.isValid()) prim ? ed_.setPrimaryColor(fromQColor(c)) : ed_.setSecondaryColor(fromQColor(c));
        return;
    }
    if (addRect().contains(pos)) {
        Rgba c = ed_.primaryColor();
        ed_.editStructure("Agregar color", ChangePalette, [c](Document& d) { d.palette.colors.push_back(c); });
        return;
    }
    int i = indexAt(pos);
    if (i < 0) return;
    Rgba c = ed_.doc().palette.colors[i];
    if (e->button() == Qt::RightButton) ed_.setSecondaryColor(c);
    else ed_.setPrimaryColor(c);
}

void PalettePanel::mouseDoubleClickEvent(QMouseEvent* e) {
    int i = indexAt(e->position().toPoint());
    if (i < 0) return;
    QColor c = QColorDialog::getColor(toQColor(ed_.doc().palette.colors[i]), this, tr("Editar color de la paleta"),
                                      QColorDialog::ShowAlphaChannel);
    if (!c.isValid()) return;
    Rgba v = fromQColor(c);
    ed_.editStructure("Editar paleta", ChangePalette, [i, v](Document& d) { d.palette.colors[i] = v; });
    ed_.setPrimaryColor(v);
}

}  // namespace xeldi::ui
