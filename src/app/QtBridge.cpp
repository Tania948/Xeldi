#include "app/QtBridge.hpp"

namespace xeldi::ui {

QImage toQImage(const Image& img) {
    QImage q(img.width(), img.height(), QImage::Format_RGBA8888);
    for (int y = 0; y < img.height(); ++y) {
        uchar* line = q.scanLine(y);
        for (int x = 0; x < img.width(); ++x) {
            Rgba c = img.pixel(x, y);
            line[x * 4 + 0] = red(c);
            line[x * 4 + 1] = green(c);
            line[x * 4 + 2] = blue(c);
            line[x * 4 + 3] = alpha(c);
        }
    }
    return q;
}

Image fromQImage(const QImage& src) {
    QImage q = src.convertToFormat(QImage::Format_RGBA8888);
    Image img(q.width(), q.height());
    for (int y = 0; y < q.height(); ++y) {
        const uchar* line = q.constScanLine(y);
        for (int x = 0; x < q.width(); ++x)
            img.setPixel(x, y, makeRgba(line[x * 4], line[x * 4 + 1], line[x * 4 + 2], line[x * 4 + 3]));
    }
    return img;
}

}  // namespace xeldi::ui
