#include "app/LayersPanel.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>

#include "app/QtBridge.hpp"
#include "app/Theme.hpp"

namespace xeldi::ui {

static QToolButton* smallButton(const QString& icon, const QString& tip, const QString& text = {}) {
    auto* b = new QToolButton;
    if (!icon.isEmpty()) b->setIcon(theme::icon(icon));
    if (!text.isEmpty()) b->setText(text);
    b->setToolTip(tip);
    b->setIconSize(QSize(16, 16));
    return b;
}

LayersPanel::LayersPanel(Editor& ed, QWidget* parent) : QWidget(parent), ed_(ed) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(6, 6, 6, 6);
    list_ = new QListWidget;
    list_->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    lay->addWidget(list_, 1);

    auto* row = new QHBoxLayout;
    auto* add = smallButton("layer_add", tr("Nueva capa"));
    auto* dup = smallButton("frame_dup", tr("Duplicar capa"));
    auto* del = smallButton("trash", tr("Eliminar capa"));
    auto* up = smallButton({}, tr("Subir capa"), QStringLiteral("▲"));
    auto* down = smallButton({}, tr("Bajar capa"), QStringLiteral("▼"));
    for (auto* b : {add, dup, del, up, down}) row->addWidget(b);
    row->addStretch();
    lay->addLayout(row);

    auto* props = new QHBoxLayout;
    props->addWidget(new QLabel(tr("Opacidad")));
    opacity_ = new QSlider(Qt::Horizontal);
    opacity_->setRange(0, 100);
    props->addWidget(opacity_, 1);
    lay->addLayout(props);
    auto* props2 = new QHBoxLayout;
    blend_ = new QComboBox;
    blend_->addItem(tr("Normal"), int(BlendMode::Normal));
    blend_->addItem(tr("Multiplicar"), int(BlendMode::Multiply));
    blend_->addItem(tr("Trama"), int(BlendMode::Screen));
    blend_->addItem(tr("Sumar"), int(BlendMode::Add));
    blend_->addItem(tr("Oscurecer"), int(BlendMode::Darken));
    blend_->addItem(tr("Aclarar"), int(BlendMode::Lighten));
    blend_->addItem(tr("Superponer"), int(BlendMode::Overlay));
    props2->addWidget(blend_, 1);
    lock_ = new QCheckBox(tr("Bloquear"));
    props2->addWidget(lock_);
    lay->addLayout(props2);

    connect(add, &QToolButton::clicked, this, [this] { ed_.addLayer(); });
    connect(dup, &QToolButton::clicked, this, [this] { ed_.duplicateLayer(); });
    connect(del, &QToolButton::clicked, this, [this] { ed_.removeLayer(); });
    connect(up, &QToolButton::clicked, this, [this] { ed_.moveLayer(+1); });
    connect(down, &QToolButton::clicked, this, [this] { ed_.moveLayer(-1); });
    connect(list_, &QListWidget::currentRowChanged, this, [this](int r) {
        if (!updating_ && r >= 0) ed_.setCurrentLayer(rowToLayer(r));
    });
    connect(list_, &QListWidget::itemChanged, this, [this](QListWidgetItem* it) {
        if (updating_) return;
        int l = rowToLayer(list_->row(it));
        const Layer& L = ed_.doc().layers.at(l);
        bool vis = it->checkState() == Qt::Checked;
        std::string name = it->text().toStdString();
        if (vis != L.visible)
            ed_.editStructure(vis ? "Mostrar capa" : "Ocultar capa", ChangeLayers,
                              [l, vis](Document& d) { d.layers[l].visible = vis; });
        else if (name != L.name && !name.empty())
            ed_.editStructure("Renombrar capa", ChangeLayers, [l, name](Document& d) { d.layers[l].name = name; });
    });
    connect(opacity_, &QSlider::sliderReleased, this, [this] {
        int l = ed_.currentLayer();
        float v = opacity_->value() / 100.0f;
        ed_.editStructure("Opacidad de capa", ChangeLayers, [l, v](Document& d) { d.layers[l].opacity = v; });
    });
    connect(blend_, &QComboBox::activated, this, [this](int i) {
        int l = ed_.currentLayer();
        auto m = BlendMode(blend_->itemData(i).toInt());
        ed_.editStructure("Modo de fusion", ChangeLayers, [l, m](Document& d) { d.layers[l].blend = m; });
    });
    connect(lock_, &QCheckBox::clicked, this, [this](bool on) {
        int l = ed_.currentLayer();
        ed_.editStructure(on ? "Bloquear capa" : "Desbloquear capa", ChangeLayers,
                          [l, on](Document& d) { d.layers[l].locked = on; });
    });

    sub_ = ed_.subscribe([this](unsigned f) {
        // Diferido: la reconstruccion puede ocurrir dentro de una senal de la propia lista.
        if (f & (ChangeLayers | ChangeDocument | ChangeHistory)) {
            if (!pending_) {
                pending_ = true;
                QMetaObject::invokeMethod(this, [this] { pending_ = false; rebuild(); }, Qt::QueuedConnection);
            }
        } else if (f & ChangeSelection) {
            syncProps();
        }
    });
    rebuild();
}

LayersPanel::~LayersPanel() { ed_.unsubscribe(sub_); }

int LayersPanel::rowToLayer(int row) const { return int(ed_.doc().layers.size()) - 1 - row; }

void LayersPanel::rebuild() {
    updating_ = true;
    list_->clear();
    const auto& layers = ed_.doc().layers;
    for (int i = int(layers.size()) - 1; i >= 0; --i) {
        const Layer& L = layers[i];
        QString label = qs(L.name);
        auto* it = new QListWidgetItem(label);
        it->setFlags(it->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
        it->setCheckState(L.visible ? Qt::Checked : Qt::Unchecked);
        it->setIcon(theme::icon(L.locked ? "lock" : (L.type == LayerType::Spatial3D ? "cube_add" : "layer_add")));
        it->setToolTip(tr("Doble clic para renombrar · casilla = visible"));
        list_->addItem(it);
    }
    updating_ = false;
    syncProps();
}

void LayersPanel::syncProps() {
    updating_ = true;
    int row = int(ed_.doc().layers.size()) - 1 - ed_.currentLayer();
    if (list_->currentRow() != row) list_->setCurrentRow(row);
    if (ed_.currentLayer() < int(ed_.doc().layers.size())) {
        const Layer& L = ed_.doc().layers[ed_.currentLayer()];
        if (!opacity_->isSliderDown()) opacity_->setValue(int(L.opacity * 100 + 0.5f));
        blend_->setCurrentIndex(blend_->findData(int(L.blend)));
        lock_->setChecked(L.locked);
    }
    updating_ = false;
}

}  // namespace xeldi::ui
