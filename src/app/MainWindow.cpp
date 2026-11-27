#include "app/MainWindow.hpp"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QImageReader>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>

#include "app/CanvasView2D.hpp"
#include "app/ExampleProject.hpp"
#include "app/LayersPanel.hpp"
#include "app/PalettePanel.hpp"
#include "app/PreviewPanel.hpp"
#include "app/QtBridge.hpp"
#include "app/ScenePanel.hpp"
#include "app/Theme.hpp"
#include "app/TimelinePanel.hpp"
#include "app/Viewport3D.hpp"
#include "io/MeshExporter.hpp"
#include "io/Png.hpp"
#include "io/SpritesheetExporter.hpp"
#include "io/XeldiFormat.hpp"

namespace xeldi::ui {

static const char* kFilter = QT_TRANSLATE_NOOP("MainWindow", "Proyecto Xeldi (*.xeldi)");

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowIcon(QIcon(":/icons/hicolor/256x256/apps/xeldi.png"));
    resize(1440, 900);
    setDockNestingEnabled(true);

    canvas_ = new CanvasView2D(ed_);
    viewport_ = new Viewport3D(ed_);
    auto* split = new QSplitter(Qt::Horizontal);
    split->addWidget(canvas_);
    split->addWidget(viewport_);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 2);
    split->setChildrenCollapsible(true);
    setCentralWidget(split);

    buildActions();
    buildToolBars();
    buildDocks();
    buildMenus();

    statusBunny_ = new QLabel;
    statusPos_ = new QLabel;
    statusInfo_ = new QLabel;
    statusBar()->addWidget(statusBunny_);
    statusBar()->addWidget(statusPos_);
    statusBar()->addWidget(statusInfo_, 1);
    connect(&bunnyTimer_, &QTimer::timeout, this, [this] {
        static const int dur[] = {6, 5, 6, 2};
        static int ticks = 0;
        if (++ticks < dur[bunnyFrame_]) return;
        ticks = 0;
        bunnyFrame_ = (bunnyFrame_ + 1) % 4;
        statusBunny_->setPixmap(QPixmap(QStringLiteral(":/mascot/bunny_idle_%1.png").arg(bunnyFrame_)));
    });
    statusBunny_->setPixmap(QPixmap(":/mascot/bunny_idle_0.png"));
    bunnyTimer_.start(40);

    connect(canvas_, &CanvasView2D::hoverPixelChanged, this, [this](int x, int y, bool inside) {
        statusPos_->setText(inside ? tr("x %1  y %2").arg(x).arg(y) : QString());
    });
    connect(canvas_, &CanvasView2D::zoomChanged, this, [this](float z) {
        statusInfo_->setText(tr("Zoom %1×").arg(double(z), 0, 'f', z < 2 ? 1 : 0));
    });
    connect(viewport_, &Viewport3D::statusText, this, [this](const QString& t) { statusPos_->setText(t); });

    connect(&playTimer_, &QTimer::timeout, this, [this] {
        double ms = double(playClock_.restart());
        ed_.tick(ms);
    });
    playTimer_.start(10);
    playClock_.start();

    sub_ = ed_.subscribe([this](unsigned f) {
        if (f & ~unsigned(ChangePixels)) refreshActions();
        if (f & (ChangeHistory | ChangeDocument | ChangeSelection)) updateTitle();
    });
    refreshActions();
    updateTitle();
}

MainWindow::~MainWindow() {
    // Los paneles se suscriben al Editor (miembro de esta clase): hay que
    // destruirlos antes que el Editor, no en el destructor de QMainWindow.
    ed_.unsubscribe(sub_);
    playTimer_.stop();
    bunnyTimer_.stop();
    for (auto* d : findChildren<QDockWidget*>()) delete d;
    delete takeCentralWidget();
}

// ---------------------------------------------------------------------------
QAction* MainWindow::act(const std::string& id) { return actions_.value(qs(id), nullptr); }

void MainWindow::registerUiAction(const std::string& id, const QString& label, const QString& shortcut,
                                  std::function<void()> fn, const QString& icon) {
    ed_.actions().add({id, label.toStdString(), shortcut.toStdString(), icon.toStdString()}, std::move(fn));
}

void MainWindow::buildActions() {
    // Acciones propias de la UI de escritorio (dialogos de archivos, etc.)
    registerUiAction("file.new", tr("Nuevo…"), "Ctrl+N", [this] { newDocument(); });
    registerUiAction("file.open", tr("Abrir…"), "Ctrl+O", [this] { openDialog(); });
    registerUiAction("file.save", tr("Guardar"), "Ctrl+S", [this] { save(); });
    registerUiAction("file.save_as", tr("Guardar como…"), "Ctrl+Shift+S", [this] { saveAs(); });
    registerUiAction("file.import_png", tr("Importar PNG en la cel actual…"), "Ctrl+I", [this] { importPng(); });
    registerUiAction("file.export_png", tr("Exportar frame como PNG…"), "Ctrl+Shift+E", [this] { exportFramePng(); });
    registerUiAction("file.export_sheet", tr("Exportar hoja de sprites…"), "", [this] { exportSpritesheet(); });
    registerUiAction("file.export_obj", tr("Exportar voxeles como OBJ…"), "", [this] { exportObj(); });
    registerUiAction("file.example", tr("Abrir ejemplo: conejito idle"), "", [this] {
        if (maybeSave()) loadExample();
    });
    registerUiAction("file.quit", tr("Salir"), "Ctrl+Q", [this] { close(); });
    registerUiAction("view.grid", tr("Cuadrícula de pixeles"), "Ctrl+'", [this] {
        canvas_->setShowGrid(!canvas_->showGrid());
        refreshActions();
    }, "grid");
    registerUiAction("view.fit", tr("Ajustar lienzo a la ventana"), "Ctrl+0", [this] { canvas_->fitToView(); });
    registerUiAction("help.about", tr("Acerca de Xeldi"), "F1", [this] { about(); });

    for (const Action& a : ed_.actions().all()) {
        auto* qa = new QAction(qs(a.info.label), this);
        if (!a.info.shortcut.empty()) qa->setShortcut(QKeySequence(qs(a.info.shortcut)));
        if (!a.info.icon.empty()) qa->setIcon(theme::icon(qs(a.info.icon)));
        qa->setCheckable(a.info.checkable || a.info.id == "view.grid");
        if (!a.info.shortcut.empty()) qa->setToolTip(qs(a.info.label) + "  (" + qs(a.info.shortcut) + ")");
        std::string id = a.info.id;
        connect(qa, &QAction::triggered, this, [this, id] { ed_.actions().trigger(id); });
        addAction(qa);  // atajos activos aunque la accion no este en un menu
        actions_.insert(qs(id), qa);
    }
    if (auto* redo = act("edit.redo")) redo->setShortcuts({QKeySequence("Ctrl+Shift+Z"), QKeySequence("Ctrl+Y")});
}

void MainWindow::refreshActions() {
    for (const Action& a : ed_.actions().all()) {
        QAction* qa = act(a.info.id);
        if (!qa) continue;
        qa->setEnabled(!a.enabled || a.enabled());
        if (a.checked) qa->setChecked(a.checked());
    }
    if (auto* g = act("view.grid")) g->setChecked(canvas_ && canvas_->showGrid());
    auto label = [](const std::string& base, const std::string& what) {
        return what.empty() ? qs(base) : qs(base) + " «" + qs(what) + "»";
    };
    if (auto* u = act("edit.undo")) u->setText(label("Deshacer", ed_.history().undoLabel()));
    if (auto* r = act("edit.redo")) r->setText(label("Rehacer", ed_.history().redoLabel()));
    if (auto* p = act("anim.play")) p->setIcon(theme::icon(ed_.isPlaying() ? "pause" : "play"));
    if (brush_ && brush_->value() != ed_.brushSize()) brush_->setValue(ed_.brushSize());
    if (pixelPerfect_) pixelPerfect_->setChecked(ed_.pixelPerfect());
}

void MainWindow::buildMenus() {
    auto addAll = [this](QMenu* m, std::initializer_list<const char*> ids) {
        for (const char* id : ids) {
            if (std::string(id) == "-") m->addSeparator();
            else if (auto* a = act(id)) m->addAction(a);
        }
    };
    addAll(menuBar()->addMenu(tr("&Archivo")),
           {"file.new", "file.open", "file.example", "-", "file.save", "file.save_as", "-", "file.import_png",
            "file.export_png", "file.export_sheet", "file.export_obj", "-", "file.quit"});
    addAll(menuBar()->addMenu(tr("&Editar")), {"edit.undo", "edit.redo", "-", "color.swap", "brush.bigger",
                                               "brush.smaller", "brush.pixel_perfect"});
    addAll(menuBar()->addMenu(tr("&Herramientas")),
           {"tool.pencil", "tool.eraser", "tool.line", "tool.rect", "tool.rect_filled", "tool.ellipse", "tool.fill",
            "tool.picker", "-", "tool3d.place", "tool3d.erase", "tool3d.paint", "tool3d.draw_plane",
            "tool3d.erase_plane", "tool3d.pick"});
    addAll(menuBar()->addMenu(tr("&Capa")),
           {"layer.add", "layer.duplicate", "layer.delete", "-", "layer.up", "layer.down"});
    addAll(menuBar()->addMenu(tr("A&nimación")), {"anim.play", "frame.prev", "frame.next", "-", "frame.add",
                                                  "frame.duplicate", "frame.delete", "-", "onion.toggle"});
    addAll(menuBar()->addMenu(tr("&3D")),
           {"plane.add_front", "plane.add_side", "plane.add_top", "plane.delete", "-", "plane.prev", "plane.next",
            "plane.forward", "plane.back", "-", "plane.extrude", "project.front", "voxels.clear"});
    auto* view = menuBar()->addMenu(tr("&Vista"));
    addAll(view, {"view.grid", "view.fit", "-", "view.front", "view.back", "view.right", "view.left", "view.top",
                  "view.iso", "view.toggle_ortho"});
    view->addSeparator();
    auto* panels = view->addMenu(tr("Paneles"));
    for (auto* d : findChildren<QDockWidget*>()) panels->addAction(d->toggleViewAction());
    addAll(menuBar()->addMenu(tr("A&yuda")), {"help.about"});
}

void MainWindow::buildToolBars() {
    auto* tools = new QToolBar(tr("Herramientas"));
    tools->setObjectName("toolsBar");
    tools->setOrientation(Qt::Vertical);
    tools->setIconSize(QSize(32, 32));
    tools->setMovable(false);
    auto sectionLabel = [](const QString& t) {
        auto* l = new QLabel(t);
        l->setObjectName("title");
        l->setAlignment(Qt::AlignCenter);
        return l;
    };
    tools->addWidget(sectionLabel("2D"));
    for (const char* id : {"tool.pencil", "tool.eraser", "tool.line", "tool.rect", "tool.rect_filled", "tool.ellipse",
                           "tool.fill", "tool.picker"})
        tools->addAction(act(id));
    tools->addSeparator();
    tools->addWidget(sectionLabel("3D"));
    for (const char* id :
         {"tool3d.place", "tool3d.erase", "tool3d.paint", "tool3d.draw_plane", "tool3d.erase_plane", "plane.extrude"})
        tools->addAction(act(id));
    addToolBar(Qt::LeftToolBarArea, tools);

    auto* opts = new QToolBar(tr("Opciones"));
    opts->setObjectName("optionsBar");
    opts->setMovable(false);
    opts->setIconSize(QSize(24, 24));
    auto* logo = new QLabel;
    logo->setPixmap(QPixmap(":/icons/hicolor/32x32/apps/xeldi.png"));
    logo->setToolTip("Xeldi");
    opts->addWidget(logo);
    auto* name = new QLabel(QStringLiteral(" Xeldi  "));
    name->setObjectName("title");
    opts->addWidget(name);
    opts->addAction(act("edit.undo"));
    opts->addAction(act("edit.redo"));
    opts->addSeparator();
    opts->addWidget(new QLabel(tr(" Pincel ")));
    brush_ = new QSpinBox;
    brush_->setRange(1, 64);
    brush_->setSuffix(" px");
    connect(brush_, &QSpinBox::valueChanged, this, [this](int v) {
        if (v != ed_.brushSize()) ed_.setBrushSize(v);
    });
    opts->addWidget(brush_);
    pixelPerfect_ = new QCheckBox(tr("Pixel perfect"));
    connect(pixelPerfect_, &QCheckBox::clicked, this, [this](bool on) { ed_.setPixelPerfect(on); });
    opts->addWidget(pixelPerfect_);
    opts->addSeparator();
    opts->addAction(act("color.swap"));
    opts->addAction(act("onion.toggle"));
    opts->addAction(act("anim.play"));
    opts->addAction(act("view.grid"));
    addToolBar(Qt::TopToolBarArea, opts);
}

void MainWindow::buildDocks() {
    auto dock = [this](const QString& title, const char* obj, QWidget* w, Qt::DockWidgetArea area) {
        auto* d = new QDockWidget(title, this);
        d->setObjectName(obj);
        d->setWidget(w);
        d->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable |
                       QDockWidget::DockWidgetClosable);
        addDockWidget(area, d);
        return d;
    };
    auto* layers = dock(tr("♥ Capas"), "dockLayers", new LayersPanel(ed_), Qt::RightDockWidgetArea);
    auto* palette = dock(tr("✿ Paleta"), "dockPalette", new PalettePanel(ed_), Qt::RightDockWidgetArea);
    auto* preview = dock(tr("▶ Preview idle"), "dockPreview", new PreviewPanel(ed_), Qt::RightDockWidgetArea);
    auto* scene = dock(tr("◆ Escena 3D"), "dockScene", new ScenePanel(ed_, viewport_), Qt::RightDockWidgetArea);
    auto* timeline = dock(tr("☰ Línea de tiempo"), "dockTimeline", new TimelinePanel(ed_), Qt::BottomDockWidgetArea);
    resizeDocks({timeline}, {190}, Qt::Vertical);
    (void)palette;
    (void)preview;
    tabifyDockWidget(layers, scene);
    layers->raise();
    for (auto* d : {layers, palette, preview, scene}) d->setMinimumWidth(290);
    resizeDocks({layers, palette, preview}, {260, 190, 260}, Qt::Vertical);
}

void MainWindow::updateTitle() {
    QString name = ed_.filePath().empty() ? qs(ed_.doc().name) : QFileInfo(qs(ed_.filePath())).fileName();
    setWindowTitle(QStringLiteral("%1%2 — Xeldi").arg(name, ed_.isModified() ? QStringLiteral(" •") : QString()));
}

// ---------------------------------------------------------------------------
// Archivos
// ---------------------------------------------------------------------------
bool MainWindow::maybeSave() {
    if (!ed_.isModified()) return true;
    auto r = QMessageBox::question(this, tr("Cambios sin guardar"),
                                   tr("«%1» tiene cambios sin guardar. ¿Guardarlos?").arg(qs(ed_.doc().name)),
                                   QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (r == QMessageBox::Save) return save();
    return r == QMessageBox::Discard;
}

void MainWindow::newDocument() {
    if (!maybeSave()) return;
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Nuevo sprite"));
    auto* form = new QFormLayout(&dlg);
    auto* w = new QSpinBox;
    auto* h = new QSpinBox;
    w->setRange(1, 4096);
    h->setRange(1, 4096);
    w->setValue(64);
    h->setValue(64);
    auto* frames = new QSpinBox;
    frames->setRange(1, 999);
    frames->setValue(1);
    form->addRow(tr("Ancho (px)"), w);
    form->addRow(tr("Alto (px)"), h);
    form->addRow(tr("Frames"), frames);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(box);
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) return;
    Document d = Document::createBlank(w->value(), h->value());
    for (int i = 1; i < frames->value(); ++i) d.insertFrame(i);
    ed_.setDocument(std::move(d));
}

bool MainWindow::openFile(const QString& path) {
    auto r = io::loadDocument(path.toStdString());
    if (!r.ok) {
        QMessageBox::warning(this, tr("No se pudo abrir"), tr("%1\n\n%2").arg(path, qs(r.error)));
        return false;
    }
    ed_.setDocument(std::move(r.document), path.toStdString());
    return true;
}

void MainWindow::loadExample() { ed_.setDocument(makeBunnyExample()); }

void MainWindow::openDialog() {
    if (!maybeSave()) return;
    QString path = QFileDialog::getOpenFileName(this, tr("Abrir proyecto"), {}, tr(kFilter));
    if (!path.isEmpty()) openFile(path);
}

bool MainWindow::save() {
    if (ed_.filePath().empty()) return saveAs();
    std::string err;
    if (!io::saveDocument(ed_.doc(), ed_.filePath(), &err)) {
        QMessageBox::warning(this, tr("No se pudo guardar"), qs(err));
        return false;
    }
    ed_.markSaved();
    statusInfo_->setText(tr("Guardado ✿"));
    return true;
}

bool MainWindow::saveAs() {
    QString path = QFileDialog::getSaveFileName(this, tr("Guardar proyecto"), qs(ed_.doc().name) + ".xeldi", tr(kFilter));
    if (path.isEmpty()) return false;
    if (!path.endsWith(".xeldi")) path += ".xeldi";
    ed_.setFilePath(path.toStdString());
    ed_.doc().name = QFileInfo(path).completeBaseName().toStdString();
    return save();
}

void MainWindow::importPng() {
    QString path = QFileDialog::getOpenFileName(this, tr("Importar imagen"), {}, tr("Imágenes (*.png *.gif *.bmp *.jpg)"));
    if (path.isEmpty()) return;
    QImage q(path);
    if (q.isNull()) {
        QMessageBox::warning(this, tr("Importar"), tr("No se pudo leer la imagen."));
        return;
    }
    Image img = fromQImage(q);
    int l = ed_.currentLayer(), f = ed_.currentFrame();
    if (ed_.doc().layers[l].locked) return;
    PixelStroke s(ed_.doc(), l, f);
    s.target().paste(img, 0, 0);
    s.markDirty(img.rect());
    ed_.commit(s.finish("Importar PNG"));
}

void MainWindow::exportFramePng() {
    QString path = QFileDialog::getSaveFileName(this, tr("Exportar frame"), qs(ed_.doc().name) + ".png", tr("PNG (*.png)"));
    if (path.isEmpty()) return;
    auto sheet = io::buildSpritesheet(ed_.doc(), {ed_.currentFrame(), ed_.currentFrame(), 1, 0, 1});
    std::string err;
    if (!io::writePngFile(sheet.image, path.toStdString(), &err)) QMessageBox::warning(this, tr("Exportar"), qs(err));
}

void MainWindow::exportSpritesheet() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Hoja de sprites"));
    auto* form = new QFormLayout(&dlg);
    auto* cols = new QSpinBox;
    cols->setRange(0, 256);
    cols->setSpecialValueText(tr("automático"));
    auto* scale = new QSpinBox;
    scale->setRange(1, 16);
    scale->setSuffix("×");
    auto* pad = new QSpinBox;
    pad->setRange(0, 64);
    form->addRow(tr("Columnas"), cols);
    form->addRow(tr("Escala"), scale);
    form->addRow(tr("Separación (px)"), pad);
    auto* box = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(box);
    connect(box, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(box, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) return;
    QString path = QFileDialog::getSaveFileName(this, tr("Exportar hoja de sprites"), qs(ed_.doc().name) + "_sheet.png",
                                                tr("PNG (*.png)"));
    if (path.isEmpty()) return;
    std::string err;
    if (!io::exportSpritesheet(ed_.doc(), path.toStdString(), {0, -1, cols->value(), pad->value(), scale->value()}, &err))
        QMessageBox::warning(this, tr("Exportar"), qs(err));
    else
        statusInfo_->setText(tr("Hoja de sprites + JSON exportados ✿"));
}

void MainWindow::exportObj() {
    if (ed_.doc().scene.voxels.empty()) {
        QMessageBox::information(this, tr("Exportar OBJ"), tr("La escena 3D no tiene voxeles todavía."));
        return;
    }
    QString path = QFileDialog::getSaveFileName(this, tr("Exportar OBJ"), qs(ed_.doc().name) + ".obj", tr("Wavefront OBJ (*.obj)"));
    if (path.isEmpty()) return;
    io::MeshStats st;
    std::string err;
    if (!io::exportObj(ed_.doc().scene.voxels, path.toStdString(), &st, &err))
        QMessageBox::warning(this, tr("Exportar OBJ"), qs(err));
    else
        statusInfo_->setText(tr("OBJ exportado: %1 caras, %2 colores").arg(st.faces).arg(st.materials));
}

void MainWindow::about() {
    QMessageBox box(this);
    box.setWindowTitle(tr("Acerca de Xeldi"));
    box.setIconPixmap(QPixmap(":/mascot/bunny_256.png").scaled(128, 128, Qt::KeepAspectRatio, Qt::FastTransformation));
    box.setText(tr("<h2>Xeldi %1</h2><p>Editor de pixel art 2D con construcción 3D en voxeles y planos de trabajo.</p>"
                   "<p>Capas, onion skin, línea de tiempo con tags y preview idle en bucle.</p>"
                   "<p>♥ Hecho con C++20 y Qt 6.</p>").arg(XELDI_VERSION));
    box.exec();
}

void MainWindow::closeEvent(QCloseEvent* e) {
    if (maybeSave()) e->accept();
    else e->ignore();
}

}  // namespace xeldi::ui
