// Prueba de humo de la interfaz: abre la ventana sin pantalla (offscreen),
// dibuja con el raton en el lienzo, usa acciones y guarda/abre un proyecto.
#include <QApplication>
#include <QDir>
#include <QDockWidget>
#include <QMouseEvent>
#include <QTimer>

#include "Test.hpp"
#include "app/CanvasView2D.hpp"
#include "app/MainWindow.hpp"
#include "app/Theme.hpp"
#include "app/Viewport3D.hpp"
#include "io/XeldiFormat.hpp"

using namespace xeldi;
using namespace xeldi::ui;

static void click(QWidget* w, QPointF p, Qt::MouseButton b = Qt::LeftButton) {
    QMouseEvent press(QEvent::MouseButtonPress, p, w->mapToGlobal(p), b, b, Qt::NoModifier);
    QApplication::sendEvent(w, &press);
    QMouseEvent move(QEvent::MouseMove, p + QPointF(30, 0), w->mapToGlobal(p + QPointF(30, 0)), Qt::NoButton, b,
                     Qt::NoModifier);
    QApplication::sendEvent(w, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, p + QPointF(30, 0), w->mapToGlobal(p + QPointF(30, 0)), b,
                        Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(w, &release);
}

static int g_argc = 1;
static char g_arg0[] = "test_ui_smoke";
static char* g_argv[] = {g_arg0, nullptr};

TEST("ventana principal: dibujar, deshacer, 3D, guardar y abrir") {
    QApplication app(g_argc, g_argv);
    theme::apply(app);
    {
        MainWindow w;
        w.resize(1280, 800);
        w.show();
        QApplication::processEvents();
        CHECK(w.findChildren<QDockWidget*>().size() == 5);
        auto* canvas = w.findChild<CanvasView2D*>();
        auto* view3d = w.findChild<Viewport3D*>();
        CHECK(canvas && view3d);
        // Pintar en el centro del lienzo con el lapiz (documento de ejemplo cargado).
        QString path = QDir::temp().filePath("xeldi_ui_smoke.xeldi");
        QFile::remove(path);
        auto actions = w.findChildren<QAction*>();
        auto trigger = [&](const QString& text) {
            for (auto* a : actions)
                if (a->text() == text) { a->trigger(); return true; }
            return false;
        };
        Editor& ed = w.editor();
        int layersBefore = int(ed.doc().layers.size());
        std::size_t voxelsBefore = ed.doc().scene.voxels.voxelCount();
        CHECK(trigger(QObject::tr("Nueva capa")));
        CHECK_EQ(int(ed.doc().layers.size()), layersBefore + 1);
        ed.setPrimaryColor(0xE86A92FFu);
        click(canvas, QPointF(canvas->width() / 2.0, canvas->height() / 2.0));
        const Image* cel = ed.doc().cel(ed.currentLayer(), ed.currentFrame());
        CHECK(cel && !cel->isFullyTransparent());
        CHECK(trigger(QObject::tr("Deshacer «Lapiz»")));
        CHECK(ed.doc().cel(ed.currentLayer(), ed.currentFrame()) == nullptr);
        click(view3d, QPointF(view3d->width() / 2.0, view3d->height() / 2.0));
        CHECK(ed.doc().scene.voxels.voxelCount() != voxelsBefore);
        click(view3d, QPointF(20, 20), Qt::RightButton);  // orbitar
        CHECK(trigger(QObject::tr("Vista isometrica")));
        CHECK(trigger(QObject::tr("Nuevo frame")));
        CHECK(trigger(QObject::tr("Reproducir")));
        QApplication::processEvents();
        CHECK(trigger(QObject::tr("Reproducir")));
        w.grab();  // fuerza el pintado de todos los widgets
        QApplication::processEvents();
        std::string err;
        CHECK(io::saveDocument(ed.doc(), path.toStdString(), &err));
        CHECK(w.openFile(path));
        CHECK_EQ(int(ed.doc().layers.size()), layersBefore + 1);
    }
}

TEST_MAIN()
