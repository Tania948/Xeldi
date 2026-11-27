#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QTimer>

#include "app/MainWindow.hpp"
#include "app/Theme.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("Xeldi");
    QApplication::setApplicationVersion(XELDI_VERSION);
    QApplication::setOrganizationName("Xeldi");
    QApplication::setDesktopFileName("io.github.tania948.Xeldi");
    QApplication::setWindowIcon(QIcon(":/icons/hicolor/256x256/apps/xeldi.png"));
    xeldi::ui::theme::apply(app);

    QCommandLineParser parser;
    parser.setApplicationDescription("Xeldi: pixel art 2D + construccion 3D en voxeles");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption shot("screenshot", "Guardar una captura de la ventana y salir (documentacion/CI)", "png");
    parser.addOption(shot);
    parser.addPositionalArgument("archivo", "Proyecto .xeldi a abrir");
    parser.process(app);

    xeldi::ui::MainWindow w;
    if (!parser.positionalArguments().isEmpty()) w.openFile(parser.positionalArguments().first());
    else w.loadExample();
    w.show();
    if (parser.isSet(shot)) {
        QString path = parser.value(shot);
        QTimer::singleShot(1200, &w, [&w, &app, path] {
            w.grab().save(path);
            app.quit();
        });
    }
    return app.exec();
}
