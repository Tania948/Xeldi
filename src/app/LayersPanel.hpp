#pragma once
#include <QWidget>

#include "editor/Editor.hpp"

class QListWidget;
class QSlider;
class QComboBox;
class QCheckBox;

namespace xeldi::ui {

class LayersPanel : public QWidget {
    Q_OBJECT
public:
    explicit LayersPanel(Editor& ed, QWidget* parent = nullptr);
    ~LayersPanel() override;

private:
    void rebuild();
    void syncProps();
    int rowToLayer(int row) const;

    Editor& ed_;
    int sub_ = 0;
    bool updating_ = false;
    bool pending_ = false;
    QListWidget* list_;
    QSlider* opacity_;
    QComboBox* blend_;
    QCheckBox* lock_;
};

}  // namespace xeldi::ui
