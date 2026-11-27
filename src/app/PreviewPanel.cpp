#include "app/PreviewPanel.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <algorithm>

#include "app/QtBridge.hpp"
#include "app/Theme.hpp"
#include "engine2d/Compositor.hpp"

namespace xeldi::ui {

void PreviewView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), theme::kWorkspace);
    if (image.isNull()) return;
    int s = std::max(1, std::min((width() - 8) / image.width(), (height() - 8) / image.height()));
    QRect r((width() - image.width() * s) / 2, (height() - image.height() * s) / 2, image.width() * s,
            image.height() * s);
    if (checker) {
        int c = std::max(4, s * 4);
        for (int y = 0; y < r.height(); y += c)
            for (int x = 0; x < r.width(); x += c)
                p.fillRect(QRect(r.x() + x, r.y() + y, std::min(c, r.width() - x), std::min(c, r.height() - y)),
                           ((x / c + y / c) % 2) ? theme::kCheckB : theme::kCheckA);
    } else {
        p.fillRect(r, Qt::white);
    }
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.drawImage(r, image);
    p.setPen(QPen(theme::kPlum, 2));
    p.drawRect(r.adjusted(-1, -1, 0, 0));
}

PreviewPanel::PreviewPanel(Editor& ed, QWidget* parent) : QWidget(parent), ed_(ed) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(6, 6, 6, 6);
    auto* row = new QHBoxLayout;
    tags_ = new QComboBox;
    row->addWidget(tags_, 1);
    auto* checker = new QCheckBox(tr("Cuadros"));
    checker->setChecked(true);
    row->addWidget(checker);
    lay->addLayout(row);
    view_ = new PreviewView;
    view_->setMinimumSize(140, 140);
    lay->addWidget(view_, 1);
    info_ = new QLabel;
    info_->setObjectName("hint");
    lay->addWidget(info_);

    connect(tags_, &QComboBox::activated, this, [this] { applySelection(); });
    connect(checker, &QCheckBox::toggled, this, [this](bool on) {
        view_->checker = on;
        view_->update();
    });
    connect(&timer_, &QTimer::timeout, this, [this] { tick(); });
    timer_.start(16);
    clock_.start();

    sub_ = ed_.subscribe([this](unsigned f) {
        if (f & (ChangeDocument | ChangeSettings | ChangeFrames | ChangeHistory)) rebuildTags();
        if (f & (ChangePixels | ChangeLayers | ChangeFrames | ChangeDocument | ChangeHistory | ChangeSettings))
            dirty_ = true;
    });
    rebuildTags();
}

PreviewPanel::~PreviewPanel() { ed_.unsubscribe(sub_); }

void PreviewPanel::rebuildTags() {
    QString current = tags_->currentData().toString();
    tags_->clear();
    const auto& tl = ed_.doc().timeline;
    for (const auto& t : tl.tags) tags_->addItem(QStringLiteral("♥ ") + qs(t.name), qs(t.name));
    tags_->addItem(tr("Toda la línea de tiempo"), QString());
    // Se conserva el tag elegido; si no habia uno, se usa el tag de preview (idle).
    int idx = current.isEmpty() ? -1 : tags_->findData(current);
    if (idx < 0 && tl.previewTag()) idx = tags_->findData(qs(tl.previewTag()->name));
    if (idx < 0) idx = tags_->count() - 1;
    tags_->setCurrentIndex(std::max(0, idx));
    applySelection();
}

void PreviewPanel::applySelection() {
    const auto& tl = ed_.doc().timeline;
    QString name = tags_->currentData().toString();
    const AnimationTag* t = name.isEmpty() ? nullptr : tl.findTag(name.toStdString());
    if (t) player_.setTag(*t);
    else player_.setRange(0, tl.frameCount() - 1);
    dirty_ = true;
}

void PreviewPanel::tick() {
    if (!isVisible()) {
        clock_.restart();
        return;
    }
    double ms = double(clock_.restart());
    if (player_.advance(ed_.doc().timeline, ms)) dirty_ = true;
    if (dirty_) render();
}

void PreviewPanel::render() {
    const auto& d = ed_.doc();
    int f = std::clamp(player_.frame(), 0, d.frameCount() - 1);
    view_->image = toQImage(composeFrame(d, f));
    view_->update();
    info_->setText(tr("frame %1 · %2 ms · %3").arg(f + 1).arg(int(player_.frameDurationMs(d.timeline, f)))
                       .arg(qs(toString(player_.mode()))));
    dirty_ = false;
}

}  // namespace xeldi::ui
