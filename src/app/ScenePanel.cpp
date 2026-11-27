#include "app/ScenePanel.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>

#include "app/QtBridge.hpp"
#include "app/Theme.hpp"
#include "app/Viewport3D.hpp"

namespace xeldi::ui {

ScenePanel::ScenePanel(Editor& ed, Viewport3D* viewport, QWidget* parent)
    : QWidget(parent), ed_(ed), viewport_(viewport) {
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* body = new QWidget;
    auto* lay = new QVBoxLayout(body);
    lay->setContentsMargins(6, 6, 6, 6);
    scroll->setWidget(body);
    outer->addWidget(scroll);

    auto button = [](const QString& text, const QString& icon = {}, const char* obj = nullptr) {
        auto* b = new QPushButton(text);
        if (!icon.isEmpty()) b->setIcon(theme::icon(icon));
        if (obj) b->setObjectName(obj);
        return b;
    };

    // --- Planos de trabajo ---
    auto* gPlanes = new QGroupBox(tr("Planos de trabajo"));
    auto* lp = new QVBoxLayout(gPlanes);
    planes_ = new QListWidget;
    planes_->setMaximumHeight(110);
    lp->addWidget(planes_);
    auto* addRow = new QHBoxLayout;
    auto* addFront = button(tr("Frente"));
    auto* addSide = button(tr("Lado"));
    auto* addTop = button(tr("Arriba"));
    auto* delPlane = button({}, "trash");
    delPlane->setToolTip(tr("Eliminar plano"));
    for (auto* b : {addFront, addSide, addTop, delPlane}) addRow->addWidget(b);
    lp->addLayout(addRow);
    auto* moveRow = new QHBoxLayout;
    auto* back = button(QStringLiteral("− 1"));
    auto* fwd = button(QStringLiteral("+ 1"));
    back->setToolTip(tr("Mover plano hacia atrás (Alt+↓)"));
    fwd->setToolTip(tr("Mover plano hacia adelante (Alt+↑)"));
    moveRow->addWidget(new QLabel(tr("Mover:")));
    moveRow->addWidget(back);
    moveRow->addWidget(fwd);
    lp->addLayout(moveRow);
    auto* exRow = new QHBoxLayout;
    exRow->addWidget(new QLabel(tr("Grosor")));
    depth_ = new QSpinBox;
    depth_->setRange(-64, 64);
    depth_->setToolTip(tr("Voxeles a lo largo de la normal (negativo = hacia atrás)"));
    exRow->addWidget(depth_);
    auto* extrude = button(tr("Extruir"), "extrude", "accent");
    exRow->addWidget(extrude, 1);
    lp->addLayout(exRow);
    lay->addWidget(gPlanes);

    // --- 3D -> 2D ---
    auto* gProj = new QGroupBox(tr("Proyectar a sprite"));
    auto* lpr = new QHBoxLayout(gProj);
    projView_ = new QComboBox;
    projView_->addItem(tr("Frente"), int(OrthoView::Front));
    projView_->addItem(tr("Atrás"), int(OrthoView::Back));
    projView_->addItem(tr("Derecha"), int(OrthoView::Right));
    projView_->addItem(tr("Izquierda"), int(OrthoView::Left));
    projView_->addItem(tr("Arriba"), int(OrthoView::Top));
    lpr->addWidget(projView_, 1);
    auto* project = button(tr("A capa nueva"), {}, "blue");
    project->setToolTip(tr("Proyecta los voxeles (1 voxel = 1 pixel) en una capa nueva del frame actual"));
    lpr->addWidget(project);
    lay->addWidget(gProj);

    // --- 2D -> 3D ---
    auto* gGen = new QGroupBox(tr("Generar desde vistas"));
    auto* lg = new QGridLayout(gGen);
    auto* hint = new QLabel(tr("Dibuja las vistas frente, lado (derecha) y arriba en frames distintos y "
                               "Xeldi reconstruye el objeto en voxeles."));
    hint->setWordWrap(true);
    hint->setObjectName("hint");
    lg->addWidget(hint, 0, 0, 1, 2);
    auto mkFrameSpin = [](int def) {
        auto* s = new QSpinBox;
        s->setRange(0, 9999);
        s->setSpecialValueText(QObject::tr("ninguna"));
        s->setValue(def);
        return s;
    };
    genFront_ = mkFrameSpin(1);
    genSide_ = mkFrameSpin(2);
    genTop_ = mkFrameSpin(3);
    genFront_->setMinimum(1);
    lg->addWidget(new QLabel(tr("Frame frente")), 1, 0);
    lg->addWidget(genFront_, 1, 1);
    lg->addWidget(new QLabel(tr("Frame lado")), 2, 0);
    lg->addWidget(genSide_, 2, 1);
    lg->addWidget(new QLabel(tr("Frame arriba")), 3, 0);
    lg->addWidget(genTop_, 3, 1);
    genReplace_ = new QCheckBox(tr("Reemplazar voxeles existentes"));
    genReplace_->setChecked(true);
    lg->addWidget(genReplace_, 4, 0, 1, 2);
    auto* gen = button(tr("Generar 3D"), "cube_add", "accent");
    lg->addWidget(gen, 5, 0, 1, 2);
    lay->addWidget(gGen);

    // --- Camara ---
    auto* gCam = new QGroupBox(tr("Cámara"));
    auto* lc = new QGridLayout(gCam);
    const std::pair<const char*, QString> views[] = {{"view.front", tr("Frente 1")}, {"view.right", tr("Lado 3")},
                                                     {"view.top", tr("Arriba 7")},   {"view.iso", tr("Iso 9")},
                                                     {"view.back", tr("Atrás")},     {"view.left", tr("Izq.")}};
    int i = 0;
    for (const auto& [id, label] : views) {
        auto* b = button(label);
        std::string sid = id;
        connect(b, &QPushButton::clicked, this, [this, sid] { ed_.actions().trigger(sid); });
        lc->addWidget(b, i / 3, i % 3);
        ++i;
    }
    auto* ortho = new QCheckBox(tr("Ortográfica (5)"));
    lc->addWidget(ortho, 2, 0, 1, 2);
    auto* scale = new QComboBox;
    scale->addItem(tr("Pixel 1x"), 1);
    scale->addItem(tr("Pixel 2x"), 2);
    scale->addItem(tr("Pixel 3x"), 3);
    scale->setCurrentIndex(1);
    scale->setToolTip(tr("Tamaño de pixel del render 3D (2x = look retro y más rápido)"));
    lc->addWidget(scale, 2, 2);
    lay->addWidget(gCam);

    auto* clear = button(tr("Vaciar voxeles"), "trash");
    lay->addWidget(clear);
    stats_ = new QLabel;
    stats_->setObjectName("hint");
    lay->addWidget(stats_);
    lay->addStretch();

    connect(planes_, &QListWidget::currentRowChanged, this, [this](int r) {
        if (updating_ || r < 0) return;
        ed_.doc().scene.activeWorkplane = r;
        ed_.notify(ChangeScene3D);
    });
    connect(planes_, &QListWidget::itemChanged, this, [this](QListWidgetItem* it) {
        if (updating_) return;
        int r = planes_->row(it);
        bool vis = it->checkState() == Qt::Checked;
        std::string name = it->text().toStdString();
        ed_.editSceneLayout("Editar plano", [r, vis, name](Document& d) {
            d.scene.workplanes[r].visible = vis;
            if (!name.empty()) d.scene.workplanes[r].name = name;
        });
    });
    connect(addFront, &QPushButton::clicked, this, [this] { ed_.addWorkplane(PlaneAxis::Front); });
    connect(addSide, &QPushButton::clicked, this, [this] { ed_.addWorkplane(PlaneAxis::Side); });
    connect(addTop, &QPushButton::clicked, this, [this] { ed_.addWorkplane(PlaneAxis::Top); });
    connect(delPlane, &QPushButton::clicked, this, [this] { ed_.removeWorkplane(); });
    connect(back, &QPushButton::clicked, this, [this] { ed_.nudgeWorkplane(-1); });
    connect(fwd, &QPushButton::clicked, this, [this] { ed_.nudgeWorkplane(+1); });
    connect(depth_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!updating_) ed_.setExtrudeDepth(v == 0 ? 1 : v);
    });
    connect(extrude, &QPushButton::clicked, this, [this] { ed_.extrudeActivePlane(); });
    connect(project, &QPushButton::clicked, this,
            [this] { ed_.projectToCel(OrthoView(projView_->currentData().toInt()), true); });
    connect(gen, &QPushButton::clicked, this, [this] {
        int side = genSide_->value() == 0 ? -1 : genSide_->value() - 1;
        int top = genTop_->value() == 0 ? -1 : genTop_->value() - 1;
        if (!ed_.generateFromFrames(genFront_->value() - 1, side, top, genReplace_->isChecked()))
            QMessageBox::information(this, tr("Generar desde vistas"),
                                     tr("El frame de la vista frontal está vacío o no existe."));
    });
    connect(ortho, &QCheckBox::clicked, this, [this] { ed_.actions().trigger("view.toggle_ortho"); });
    connect(scale, &QComboBox::activated, this, [this, scale] { viewport_->setPixelScale(scale->currentData().toInt()); });
    connect(clear, &QPushButton::clicked, this, [this] { ed_.clearVoxels(); });

    sub_ = ed_.subscribe([this, ortho](unsigned f) {
        if (!(f & (ChangeScene3D | ChangeDocument | ChangeHistory | ChangeSelection))) return;
        ortho->setChecked(ed_.doc().scene.camera.ortho);
        if (!pending_) {
            pending_ = true;
            QMetaObject::invokeMethod(this, [this] { pending_ = false; sync(); }, Qt::QueuedConnection);
        }
    });
    sync();
}

ScenePanel::~ScenePanel() { ed_.unsubscribe(sub_); }

void ScenePanel::sync() {
    updating_ = true;
    const auto& sc = ed_.doc().scene;
    if (planes_->count() != int(sc.workplanes.size())) planes_->clear();
    for (int i = 0; i < int(sc.workplanes.size()); ++i) {
        const auto& w = sc.workplanes[i];
        QListWidgetItem* it = i < planes_->count() ? planes_->item(i) : nullptr;
        if (!it) {
            it = new QListWidgetItem;
            it->setFlags(it->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
            planes_->addItem(it);
        }
        it->setText(qs(w.name));
        it->setCheckState(w.visible ? Qt::Checked : Qt::Unchecked);
        it->setIcon(theme::icon("plane_draw"));
    }
    planes_->setCurrentRow(sc.activeWorkplane);
    depth_->setValue(ed_.extrudeDepth());
    int nf = ed_.doc().frameCount();
    genFront_->setMaximum(nf);
    genSide_->setMaximum(nf);
    genTop_->setMaximum(nf);
    stats_->setText(tr("%1 voxeles · %2 chunks de 16³").arg(sc.voxels.voxelCount()).arg(sc.voxels.chunkCount()));
    updating_ = false;
}

}  // namespace xeldi::ui
