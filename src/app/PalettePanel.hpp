#pragma once
#include <QWidget>

#include "editor/Editor.hpp"

namespace xeldi::ui {

/// Colores primario/secundario + rejilla de la paleta del documento.
class PalettePanel : public QWidget {
    Q_OBJECT
public:
    explicit PalettePanel(Editor& ed, QWidget* parent = nullptr);
    ~PalettePanel() override;
    QSize sizeHint() const override { return {220, 200}; }
    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int w) const override;

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override;

private:
    int indexAt(QPoint p) const;
    QRect swatchRect(int i) const;
    QRect primaryRect() const { return {10, 10, 34, 34}; }
    QRect secondaryRect() const { return {30, 28, 34, 34}; }
    QRect addRect() const { return {width() - 40, 18, 30, 26}; }
    int columns() const;

    Editor& ed_;
    int sub_ = 0;
    static constexpr int kTop = 72, kCell = 22;
};

}  // namespace xeldi::ui
