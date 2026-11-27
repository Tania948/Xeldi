#pragma once
// Linea de tiempo: rejilla capas x frames, tags de animacion, reproduccion
// y ajustes de onion skin.
#include <QWidget>

#include "editor/Editor.hpp"

class QSpinBox;
class QSlider;
class QToolButton;
class QCheckBox;

namespace xeldi::ui {

class FrameGrid : public QWidget {
    Q_OBJECT
public:
    explicit FrameGrid(Editor& ed, QWidget* parent = nullptr);
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int selectionFrom() const { return std::min(selA_, selB_); }
    int selectionTo() const { return std::max(selA_, selB_); }

signals:
    void tagDoubleClicked(int index);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;

private:
    int frameAt(int x) const;
    int layerAt(int y) const;
    int tagAt(QPoint p) const;
    static constexpr int kNameW = 110, kCell = 22, kTagH = 16, kHeaderH = 18;
    Editor& ed_;
    int selA_ = 0, selB_ = 0;
};

class TimelinePanel : public QWidget {
    Q_OBJECT
public:
    explicit TimelinePanel(Editor& ed, QWidget* parent = nullptr);
    ~TimelinePanel() override;

private:
    void sync();
    void editTag(int index);
    void newTag();

    Editor& ed_;
    int sub_ = 0;
    bool updating_ = false;
    FrameGrid* grid_;
    QToolButton* play_;
    QSpinBox* fps_;
    QSpinBox* duration_;
    QToolButton* onion_;
    QSpinBox* onionPrev_;
    QSpinBox* onionNext_;
    QSlider* onionOpacity_;
};

}  // namespace xeldi::ui
