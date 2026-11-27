#include "app/TimelinePanel.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

#include "app/QtBridge.hpp"
#include "app/Theme.hpp"

namespace xeldi::ui {

// ---------------------------------------------------------------------------
FrameGrid::FrameGrid(Editor& ed, QWidget* parent) : QWidget(parent), ed_(ed) {
    setMouseTracking(false);
}

QSize FrameGrid::sizeHint() const {
    const auto& d = ed_.doc();
    return {kNameW + d.frameCount() * kCell + 10, kTagH + kHeaderH + int(d.layers.size()) * kCell + 4};
}
QSize FrameGrid::minimumSizeHint() const { return sizeHint(); }

int FrameGrid::frameAt(int x) const {
    if (x < kNameW) return -1;
    int f = (x - kNameW) / kCell;
    return f < ed_.doc().frameCount() ? f : -1;
}
int FrameGrid::layerAt(int y) const {
    int row = (y - kTagH - kHeaderH) / kCell;
    if (y < kTagH + kHeaderH || row >= int(ed_.doc().layers.size())) return -1;
    return int(ed_.doc().layers.size()) - 1 - row;
}
int FrameGrid::tagAt(QPoint p) const {
    if (p.y() >= kTagH) return -1;
    int f = frameAt(p.x());
    const auto& tags = ed_.doc().timeline.tags;
    for (int i = int(tags.size()) - 1; i >= 0; --i)
        if (f >= tags[i].from && f <= tags[i].to) return i;
    return -1;
}

void FrameGrid::paintEvent(QPaintEvent*) {
    const auto& d = ed_.doc();
    QPainter p(this);
    p.fillRect(rect(), QColor(0xFF, 0xF5, 0xF8));
    int nL = int(d.layers.size()), nF = d.frameCount();
    int top = kTagH + kHeaderH;
    // Seleccion de rango
    int s0 = selectionFrom(), s1 = selectionTo();
    if (s1 > s0)
        p.fillRect(QRect(kNameW + s0 * kCell, kTagH, (s1 - s0 + 1) * kCell, kHeaderH + nL * kCell),
                   QColor(0xB8, 0xC4, 0xFF, 90));
    // Tags
    for (const auto& t : d.timeline.tags) {
        QRect r(kNameW + t.from * kCell + 1, 1, (t.to - t.from + 1) * kCell - 2, kTagH - 2);
        p.setPen(QPen(theme::kPlum, 1));
        p.setBrush(toQColor(t.color));
        p.drawRoundedRect(r, 5, 5);
        p.setPen(theme::kText);
        QString label = qs(t.name) + (t.autoPreview ? QStringLiteral(" ♥") : QString());
        p.drawText(r.adjusted(4, 0, -2, 0), Qt::AlignVCenter | Qt::AlignLeft, label);
    }
    // Encabezado de frames
    for (int f = 0; f < nF; ++f) {
        QRect r(kNameW + f * kCell, kTagH, kCell, kHeaderH);
        bool cur = f == ed_.currentFrame();
        if (cur) p.fillRect(r, theme::kPink);
        p.setPen(cur ? Qt::white : theme::kText);
        p.drawText(r, Qt::AlignCenter, QString::number(f + 1));
    }
    // Filas de capas
    for (int row = 0; row < nL; ++row) {
        int l = nL - 1 - row;
        const Layer& L = d.layers[l];
        QRect nameR(0, top + row * kCell, kNameW, kCell);
        bool curL = l == ed_.currentLayer();
        p.fillRect(nameR, curL ? theme::kPanel : theme::kPanelAlt);
        p.setPen(L.visible ? theme::kText : QColor(0xB9, 0xA5, 0x9A));
        p.drawText(nameR.adjusted(6, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft,
                   p.fontMetrics().elidedText(qs(L.name), Qt::ElideRight, kNameW - 10));
        for (int f = 0; f < nF; ++f) {
            QRect c(kNameW + f * kCell, top + row * kCell, kCell, kCell);
            bool cur = f == ed_.currentFrame();
            p.fillRect(c, cur ? QColor(0xFF, 0xE3, 0xEC) : (curL ? QColor(0xFF, 0xF0, 0xF5) : QColor(0xFF, 0xFA, 0xFC)));
            p.setPen(QColor(0xE2, 0xD3, 0xC7));
            p.setBrush(Qt::NoBrush);
            p.drawRect(c.adjusted(0, 0, -1, -1));
            const Image* cel = d.cel(l, f);
            bool filled = cel && !cel->isFullyTransparent();
            p.setPen(QPen(theme::kPlum, 1.5));
            p.setBrush(filled ? (curL && cur ? theme::kPinkStrong : theme::kPlum) : Qt::NoBrush);
            p.setRenderHint(QPainter::Antialiasing, true);
            p.drawEllipse(c.center(), 4, 4);
            p.setRenderHint(QPainter::Antialiasing, false);
        }
    }
    // Cursor del frame actual
    int cx = kNameW + ed_.currentFrame() * kCell;
    p.setPen(QPen(theme::kPinkStrong, 2));
    p.setBrush(Qt::NoBrush);
    p.drawRect(QRect(cx, kTagH, kCell, kHeaderH + nL * kCell).adjusted(1, 1, -1, -1));
}

void FrameGrid::mousePressEvent(QMouseEvent* e) {
    QPoint p = e->position().toPoint();
    int f = frameAt(p.x()), l = layerAt(p.y());
    if (f >= 0) {
        if (e->modifiers() & Qt::ShiftModifier) selB_ = f;
        else selA_ = selB_ = f;
        if (ed_.isPlaying()) ed_.setPlaying(false);
        ed_.setCurrentFrame(f);
    }
    if (l >= 0) ed_.setCurrentLayer(l);
    update();
}

void FrameGrid::mouseMoveEvent(QMouseEvent* e) {
    int f = frameAt(int(e->position().x()));
    if (f >= 0 && (e->buttons() & Qt::LeftButton)) {
        selB_ = f;
        ed_.setCurrentFrame(f);
        update();
    }
}

void FrameGrid::mouseDoubleClickEvent(QMouseEvent* e) {
    int t = tagAt(e->position().toPoint());
    if (t >= 0) emit tagDoubleClicked(t);
}

// ---------------------------------------------------------------------------
TimelinePanel::TimelinePanel(Editor& ed, QWidget* parent) : QWidget(parent), ed_(ed) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(6, 6, 6, 6);
    auto* bar = new QHBoxLayout;
    auto tb = [&](const QString& icon, const QString& text, const QString& tip) {
        auto* b = new QToolButton;
        if (!icon.isEmpty()) b->setIcon(theme::icon(icon));
        if (!text.isEmpty()) b->setText(text);
        b->setToolTip(tip);
        b->setIconSize(QSize(16, 16));
        bar->addWidget(b);
        return b;
    };
    auto* prev = tb({}, QStringLiteral("◀"), tr("Frame anterior ( , )"));
    play_ = tb("play", {}, tr("Reproducir / pausar (Enter)"));
    play_->setCheckable(true);
    auto* next = tb({}, QStringLiteral("▶"), tr("Frame siguiente ( . )"));
    bar->addSpacing(8);
    auto* add = tb("frame_add", {}, tr("Nuevo frame (Alt+N)"));
    auto* dup = tb("frame_dup", {}, tr("Duplicar frame (Alt+D)"));
    auto* del = tb("trash", {}, tr("Eliminar frame"));
    bar->addSpacing(8);
    auto* tagBtn = new QPushButton(tr("♥ Tag desde selección"));
    tagBtn->setObjectName("lavender");
    tagBtn->setToolTip(tr("Selecciona un rango de frames (clic y arrastra, o Shift+clic) y crea un tag como «idle»"));
    bar->addWidget(tagBtn);
    bar->addSpacing(12);
    bar->addWidget(new QLabel(tr("FPS")));
    fps_ = new QSpinBox;
    fps_->setRange(1, 60);
    bar->addWidget(fps_);
    bar->addWidget(new QLabel(tr("Duración (ms)")));
    duration_ = new QSpinBox;
    duration_->setRange(10, 10000);
    duration_->setSingleStep(10);
    bar->addWidget(duration_);
    bar->addSpacing(12);
    onion_ = tb("onion", {}, tr("Onion skin (F3)"));
    onion_->setCheckable(true);
    bar->addWidget(new QLabel(tr("antes")));
    onionPrev_ = new QSpinBox;
    onionPrev_->setRange(0, 8);
    bar->addWidget(onionPrev_);
    bar->addWidget(new QLabel(tr("después")));
    onionNext_ = new QSpinBox;
    onionNext_->setRange(0, 8);
    bar->addWidget(onionNext_);
    onionOpacity_ = new QSlider(Qt::Horizontal);
    onionOpacity_->setRange(5, 90);
    onionOpacity_->setMaximumWidth(90);
    onionOpacity_->setToolTip(tr("Opacidad del onion skin"));
    bar->addWidget(onionOpacity_);
    bar->addStretch();
    lay->addLayout(bar);

    grid_ = new FrameGrid(ed_);
    auto* scroll = new QScrollArea;
    scroll->setWidget(grid_);
    scroll->setWidgetResizable(false);
    scroll->setFrameShape(QFrame::NoFrame);
    lay->addWidget(scroll, 1);
    setMinimumHeight(150);

    connect(prev, &QToolButton::clicked, this, [this] { ed_.actions().trigger("frame.prev"); });
    connect(next, &QToolButton::clicked, this, [this] { ed_.actions().trigger("frame.next"); });
    connect(play_, &QToolButton::clicked, this, [this] { ed_.actions().trigger("anim.play"); });
    connect(add, &QToolButton::clicked, this, [this] { ed_.addFrame(); });
    connect(dup, &QToolButton::clicked, this, [this] { ed_.duplicateFrame(); });
    connect(del, &QToolButton::clicked, this, [this] { ed_.removeFrame(); });
    connect(tagBtn, &QPushButton::clicked, this, [this] { newTag(); });
    connect(grid_, &FrameGrid::tagDoubleClicked, this, [this](int i) { editTag(i); });
    connect(fps_, &QSpinBox::editingFinished, this, [this] {
        int v = fps_->value();
        if (v != ed_.doc().timeline.fps)
            ed_.editStructure("FPS", ChangeSettings, [v](Document& d) { d.timeline.fps = v; });
    });
    connect(duration_, &QSpinBox::editingFinished, this, [this] {
        int v = duration_->value(), f = ed_.currentFrame();
        if (v != ed_.doc().timeline.frames[f].durationMs)
            ed_.editStructure("Duracion de frame", ChangeFrames, [v, f](Document& d) { d.timeline.frames[f].durationMs = v; });
    });
    connect(onion_, &QToolButton::clicked, this, [this] { ed_.actions().trigger("onion.toggle"); });
    connect(onionPrev_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!updating_) ed_.editStructure("Onion skin", ChangeSettings, [v](Document& d) { d.onion.prevFrames = v; });
    });
    connect(onionNext_, &QSpinBox::valueChanged, this, [this](int v) {
        if (!updating_) ed_.editStructure("Onion skin", ChangeSettings, [v](Document& d) { d.onion.nextFrames = v; });
    });
    connect(onionOpacity_, &QSlider::sliderReleased, this, [this] {
        float v = onionOpacity_->value() / 100.0f;
        ed_.editStructure("Onion skin", ChangeSettings, [v](Document& d) { d.onion.baseOpacity = v; });
    });

    sub_ = ed_.subscribe([this](unsigned) { sync(); });
    sync();
}

TimelinePanel::~TimelinePanel() { ed_.unsubscribe(sub_); }

void TimelinePanel::sync() {
    updating_ = true;
    const auto& d = ed_.doc();
    grid_->resize(grid_->sizeHint());
    grid_->update();
    play_->setChecked(ed_.isPlaying());
    play_->setIcon(theme::icon(ed_.isPlaying() ? "pause" : "play"));
    if (!fps_->hasFocus()) fps_->setValue(d.timeline.fps);
    if (!duration_->hasFocus()) duration_->setValue(d.timeline.frames[ed_.currentFrame()].durationMs);
    onion_->setChecked(d.onion.enabled);
    onionPrev_->setValue(d.onion.prevFrames);
    onionNext_->setValue(d.onion.nextFrames);
    if (!onionOpacity_->isSliderDown()) onionOpacity_->setValue(int(d.onion.baseOpacity * 100 + 0.5f));
    updating_ = false;
}

void TimelinePanel::newTag() {
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Nuevo tag de animación"),
                                         tr("Nombre (frames %1–%2):").arg(grid_->selectionFrom() + 1).arg(grid_->selectionTo() + 1),
                                         QLineEdit::Normal, ed_.doc().timeline.tags.empty() ? "idle" : "walk", &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    ed_.addTagFromSelection(name.trimmed().toStdString(), grid_->selectionFrom(), grid_->selectionTo());
}

void TimelinePanel::editTag(int index) {
    if (index < 0 || index >= int(ed_.doc().timeline.tags.size())) return;
    AnimationTag t = ed_.doc().timeline.tags[index];
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Editar tag"));
    auto* form = new QFormLayout(&dlg);
    auto* name = new QLineEdit(qs(t.name));
    auto* from = new QSpinBox;
    auto* to = new QSpinBox;
    from->setRange(1, ed_.doc().frameCount());
    to->setRange(1, ed_.doc().frameCount());
    from->setValue(t.from + 1);
    to->setValue(t.to + 1);
    auto* loop = new QComboBox;
    loop->addItem(tr("Hacia adelante"), int(LoopMode::Forward));
    loop->addItem(tr("En reversa"), int(LoopMode::Reverse));
    loop->addItem(tr("Ping-pong"), int(LoopMode::PingPong));
    loop->setCurrentIndex(loop->findData(int(t.loop)));
    auto* fps = new QSpinBox;
    fps->setRange(0, 60);
    fps->setSpecialValueText(tr("usar duraciones"));
    fps->setValue(t.fpsOverride);
    auto* preview = new QCheckBox(tr("Reproducir en la preview idle"));
    preview->setChecked(t.autoPreview);
    form->addRow(tr("Nombre"), name);
    form->addRow(tr("Desde frame"), from);
    form->addRow(tr("Hasta frame"), to);
    form->addRow(tr("Bucle"), loop);
    form->addRow(tr("FPS propio"), fps);
    form->addRow(preview);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    auto* remove = box->addButton(tr("Eliminar tag"), QDialogButtonBox::DestructiveRole);
    form->addRow(box);
    bool removeTag = false;
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(remove, &QPushButton::clicked, &dlg, [&] { removeTag = true; dlg.accept(); });
    if (dlg.exec() != QDialog::Accepted) return;
    if (removeTag) {
        ed_.editStructure("Eliminar tag", ChangeSettings | ChangeFrames,
                          [index](Document& d) { d.timeline.tags.erase(d.timeline.tags.begin() + index); });
        return;
    }
    t.name = name->text().toStdString();
    t.from = std::min(from->value(), to->value()) - 1;
    t.to = std::max(from->value(), to->value()) - 1;
    t.loop = LoopMode(loop->currentData().toInt());
    t.fpsOverride = fps->value();
    t.autoPreview = preview->isChecked();
    ed_.editStructure("Editar tag", ChangeSettings | ChangeFrames, [index, t](Document& d) {
        if (t.autoPreview)
            for (auto& o : d.timeline.tags) o.autoPreview = false;
        d.timeline.tags[index] = t;
    });
}

}  // namespace xeldi::ui
