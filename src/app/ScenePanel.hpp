#pragma once
// Panel de la escena 3D: planos de trabajo, extrusion, proyeccion a sprite,
// «generar desde vistas» y camara.
#include <QWidget>

#include "editor/Editor.hpp"

class QListWidget;
class QSpinBox;
class QComboBox;
class QCheckBox;
class QLabel;

namespace xeldi::ui {

class Viewport3D;

class ScenePanel : public QWidget {
    Q_OBJECT
public:
    ScenePanel(Editor& ed, Viewport3D* viewport, QWidget* parent = nullptr);
    ~ScenePanel() override;

private:
    void sync();

    Editor& ed_;
    Viewport3D* viewport_;
    int sub_ = 0;
    bool updating_ = false, pending_ = false;
    QListWidget* planes_;
    QSpinBox* depth_;
    QComboBox* projView_;
    QSpinBox* genFront_;
    QSpinBox* genSide_;
    QSpinBox* genTop_;
    QCheckBox* genReplace_;
    QLabel* stats_;
};

}  // namespace xeldi::ui
