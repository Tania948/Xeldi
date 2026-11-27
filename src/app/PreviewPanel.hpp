#pragma once
// Ventana de preview: reproduce en bucle un tag (p. ej. «idle") a su FPS real
// mientras se edita otro frame en el lienzo principal.
#include <QElapsedTimer>
#include <QImage>
#include <QTimer>
#include <QWidget>

#include "editor/Editor.hpp"
#include "engine2d/AnimationPlayer.hpp"

class QComboBox;
class QLabel;

namespace xeldi::ui {

class PreviewView : public QWidget {
public:
    using QWidget::QWidget;
    QImage image;
    bool checker = true;

protected:
    void paintEvent(QPaintEvent*) override;
};

class PreviewPanel : public QWidget {
    Q_OBJECT
public:
    explicit PreviewPanel(Editor& ed, QWidget* parent = nullptr);
    ~PreviewPanel() override;

private:
    void rebuildTags();
    void applySelection();
    void tick();
    void render();

    Editor& ed_;
    int sub_ = 0;
    AnimationPlayer player_;
    QTimer timer_;
    QElapsedTimer clock_;
    QComboBox* tags_;
    QLabel* info_;
    PreviewView* view_;
    bool dirty_ = true;
};

}  // namespace xeldi::ui
