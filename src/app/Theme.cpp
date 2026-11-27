#include "app/Theme.hpp"

#include <QApplication>
#include <QFont>
#include <QPalette>
#include <QPixmap>
#include <QStyleFactory>

namespace xeldi::ui::theme {

QString styleSheet() {
    return QStringLiteral(R"QSS(
* { color: #4A3344; }
QMainWindow, QDialog { background: #FBEFF3; }
QMainWindow::separator { background: #E2D3C7; width: 4px; height: 4px; }

QMenuBar { background: #F7D6E0; border-bottom: 2px solid #5A3D52; padding: 2px; }
QMenuBar::item { padding: 4px 10px; border-radius: 6px; background: transparent; }
QMenuBar::item:selected { background: #F2B8CC; }
QMenu { background: #FFF8FA; border: 2px solid #5A3D52; border-radius: 8px; padding: 4px; }
QMenu::item { padding: 5px 22px 5px 10px; border-radius: 6px; }
QMenu::item:selected { background: #F7D6E0; }
QMenu::item:disabled { color: #B9A59A; }
QMenu::separator { height: 2px; background: #E2D3C7; margin: 4px 6px; }

QToolBar { background: #F7D6E0; border: none; spacing: 3px; padding: 4px; }
QToolBar#toolsBar { border-right: 2px solid #5A3D52; }
QToolBar#optionsBar { border-bottom: 2px solid #5A3D52; }
QToolButton { background: transparent; border: 2px solid transparent; border-radius: 7px; padding: 3px; }
QToolButton:hover { background: #FFF0F5; border-color: #E6B3B8; }
QToolButton:checked { background: #FFFFFF; border: 2px solid #5A3D52; border-bottom: 3px solid #5A3D52; }
QToolButton:pressed { background: #F2B8CC; }

QDockWidget { titlebar-close-icon: none; font-weight: bold; }
QDockWidget::title {
  background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #F2B8CC, stop:1 #C9A8E8);
  border: 2px solid #5A3D52; border-bottom: none; border-top-left-radius: 8px; border-top-right-radius: 8px;
  padding: 4px 8px; text-align: left;
}
QDockWidget > QWidget { background: #FFF5F8; border: 2px solid #5A3D52; border-top: none; }

QPushButton {
  background: #FFFFFF; border: 2px solid #5A3D52; border-bottom: 4px solid #5A3D52; border-radius: 8px;
  padding: 3px 10px; min-height: 18px;
}
QPushButton:hover { background: #FFF0F5; }
QPushButton:pressed { background: #F2B8CC; border-bottom-width: 2px; margin-top: 2px; }
QPushButton:checked { background: #F2B8CC; }
QPushButton:disabled { color: #B9A59A; border-color: #C8AD9C; background: #F3EEEC; }
QPushButton#accent { background: #E86A92; color: white; font-weight: bold; }
QPushButton#accent:hover { background: #F07FA3; }
QPushButton#blue { background: #B8C4FF; }
QPushButton#lavender { background: #E3D4F7; }

QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {
  background: #FFFFFF; border: 2px solid #C8AD9C; border-radius: 7px; padding: 2px 6px; min-height: 18px;
  selection-background-color: #F2B8CC; selection-color: #4A3344;
}
QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: #E86A92; }
QComboBox::drop-down { border: none; width: 18px; }
QComboBox QAbstractItemView { background: #FFF8FA; border: 2px solid #5A3D52; selection-background-color: #F7D6E0; }
QSpinBox::up-button, QSpinBox::down-button { width: 14px; border: none; background: #F7D6E0; }

QListWidget {
  background: #FFFFFF; border: 2px solid #C8AD9C; border-radius: 8px; padding: 2px; outline: none;
}
QListWidget::item { padding: 4px; border-radius: 6px; margin: 1px; }
QListWidget::item:selected { background: #F7D6E0; color: #4A3344; border: 2px solid #E86A92; }
QListWidget::item:hover { background: #FFF0F5; }

QSlider::groove:horizontal { height: 6px; background: #E2D3C7; border-radius: 3px; }
QSlider::sub-page:horizontal { background: #F29BB8; border-radius: 3px; }
QSlider::handle:horizontal {
  background: #FFFFFF; border: 2px solid #5A3D52; width: 12px; height: 12px; margin: -5px 0; border-radius: 4px;
}
QCheckBox::indicator { width: 14px; height: 14px; border: 2px solid #5A3D52; border-radius: 4px; background: #FFFFFF; }
QCheckBox::indicator:checked { background: #E86A92; }

QGroupBox {
  border: 2px solid #E2D3C7; border-radius: 10px; margin-top: 14px; padding: 8px 6px 6px 6px; background: #FFFAFC;
}
QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 6px; color: #A775C9; font-weight: bold; }

QScrollBar:vertical { background: #F3EEEC; width: 12px; margin: 0; border-radius: 6px; }
QScrollBar::handle:vertical { background: #C8AD9C; border-radius: 6px; min-height: 24px; }
QScrollBar:horizontal { background: #F3EEEC; height: 12px; margin: 0; border-radius: 6px; }
QScrollBar::handle:horizontal { background: #C8AD9C; border-radius: 6px; min-width: 24px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }

QStatusBar { background: #E2D3C7; border-top: 2px solid #5A3D52; }
QStatusBar QLabel { padding: 0 6px; }
QSplitter::handle { background: #E2D3C7; }
QSplitter::handle:horizontal { width: 6px; }
QToolTip { background: #FFF8FA; border: 2px solid #5A3D52; border-radius: 6px; padding: 4px; color: #4A3344; }
QTabBar::tab { background: #F3E6E9; border: 2px solid #C8AD9C; border-bottom: none; padding: 4px 12px;
  border-top-left-radius: 8px; border-top-right-radius: 8px; margin-right: 2px; }
QTabBar::tab:selected { background: #F7D6E0; border-color: #5A3D52; }
QLabel#hint { color: #8C7480; }
QLabel#title { color: #A775C9; font-weight: bold; }
)QSS");
}

void apply(QApplication& app) {
    app.setStyle(QStyleFactory::create("Fusion"));
    QPalette p;
    p.setColor(QPalette::Window, kBg);
    p.setColor(QPalette::WindowText, kText);
    p.setColor(QPalette::Base, Qt::white);
    p.setColor(QPalette::AlternateBase, kPanelAlt);
    p.setColor(QPalette::Text, kText);
    p.setColor(QPalette::Button, kPanel);
    p.setColor(QPalette::ButtonText, kText);
    p.setColor(QPalette::Highlight, kPink);
    p.setColor(QPalette::HighlightedText, kText);
    p.setColor(QPalette::ToolTipBase, QColor(0xFF, 0xF8, 0xFA));
    p.setColor(QPalette::ToolTipText, kText);
    app.setPalette(p);
    QFont f = app.font();
    f.setFamilies({"Nunito", "Quicksand", "Ubuntu", "Cantarell", "Noto Sans", "DejaVu Sans"});
    f.setPointSize(10);
    app.setFont(f);
    app.setStyleSheet(styleSheet());
}

QIcon icon(const QString& name) {
    QPixmap px(QStringLiteral(":/ui/%1.png").arg(name));
    if (px.isNull()) return {};
    QIcon ic;
    for (int s : {1, 2, 3}) ic.addPixmap(px.scaled(16 * s, 16 * s, Qt::KeepAspectRatio, Qt::FastTransformation));
    return ic;
}

}  // namespace xeldi::ui::theme
