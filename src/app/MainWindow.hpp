#pragma once
#include <QElapsedTimer>
#include <QMainWindow>
#include <QMap>
#include <QTimer>

#include "editor/Editor.hpp"

class QAction;
class QLabel;
class QSpinBox;
class QCheckBox;

namespace xeldi::ui {

class CanvasView2D;
class Viewport3D;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    bool openFile(const QString& path);
    void loadExample();
    Editor& editor() { return ed_; }

protected:
    void closeEvent(QCloseEvent*) override;

private:
    void buildActions();
    void buildMenus();
    void buildToolBars();
    void buildDocks();
    void refreshActions();
    void updateTitle();
    QAction* act(const std::string& id);
    void registerUiAction(const std::string& id, const QString& label, const QString& shortcut,
                          std::function<void()> fn, const QString& icon = {});

    bool maybeSave();
    void newDocument();
    void openDialog();
    bool save();
    bool saveAs();
    void importPng();
    void exportFramePng();
    void exportSpritesheet();
    void exportObj();
    void about();

    Editor ed_;
    int sub_ = 0;
    CanvasView2D* canvas_ = nullptr;
    Viewport3D* viewport_ = nullptr;
    QMap<QString, QAction*> actions_;
    QLabel* statusPos_ = nullptr;
    QLabel* statusInfo_ = nullptr;
    QLabel* statusBunny_ = nullptr;
    QSpinBox* brush_ = nullptr;
    QCheckBox* pixelPerfect_ = nullptr;
    QTimer playTimer_;
    QElapsedTimer playClock_;
    QTimer bunnyTimer_;
    int bunnyFrame_ = 0;
};

}  // namespace xeldi::ui
