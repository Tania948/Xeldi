#pragma once
// Tema visual de Xeldi: retro + cute, pastel. Rosa de base, cafe clarito para
// lo menos saturado y toques de azul y lavanda.
#include <QColor>
#include <QIcon>
#include <QString>

class QApplication;

namespace xeldi::ui::theme {

inline const QColor kBg{0xFB, 0xEF, 0xF3};        // fondo general
inline const QColor kPanel{0xF7, 0xD6, 0xE0};     // paneles
inline const QColor kPanelAlt{0xF3, 0xE6, 0xE9};  // petalo
inline const QColor kCafe{0xE2, 0xD3, 0xC7};      // cafe clarito
inline const QColor kCafeDark{0xC8, 0xAD, 0x9C};
inline const QColor kPink{0xF2, 0x9B, 0xB8};
inline const QColor kPinkStrong{0xE8, 0x6A, 0x92};
inline const QColor kBlue{0xB8, 0xC4, 0xFF};
inline const QColor kLavender{0xC9, 0xA8, 0xE8};
inline const QColor kPlum{0x5A, 0x3D, 0x52};      // contornos
inline const QColor kText{0x4A, 0x33, 0x44};
inline const QColor kCheckA{0xFF, 0xF8, 0xFA};
inline const QColor kCheckB{0xF6, 0xE4, 0xEA};
inline const QColor kWorkspace{0xEB, 0xD9, 0xE0};

void apply(QApplication& app);
QString styleSheet();
/// Icono pixel art de :/ui/<name>.png escalado sin suavizado.
QIcon icon(const QString& name);

}  // namespace xeldi::ui::theme
