#pragma once
// Conversiones entre el motor (sin Qt) y Qt. Solo la capa de UI conoce Qt.
#include <QColor>
#include <QImage>
#include <QString>

#include "core/Image.hpp"

namespace xeldi::ui {

inline QColor toQColor(Rgba c) { return QColor(red(c), green(c), blue(c), alpha(c)); }
inline Rgba fromQColor(const QColor& c) { return makeRgba(c.red(), c.green(), c.blue(), c.alpha()); }
QImage toQImage(const Image& img);
Image fromQImage(const QImage& img);
inline QString qs(const std::string& s) { return QString::fromStdString(s); }

}  // namespace xeldi::ui
