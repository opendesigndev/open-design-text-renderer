
#include "BitmapWriter.h"

namespace odtr {
namespace priv {

BitmapWriter::BitmapWriter(compat::BitmapRGBA& bitmap)
    : bitmap_(bitmap)
{ }

bool BitmapWriter::checkV(int y) {
    return y >= 0 && y < bitmap_.height();
}

bool BitmapWriter::checkH(int x) {
    return x >= 0 && x < bitmap_.width();
}

void BitmapWriter::write(int x, int y, compat::Pixel32 color) {
    if (checkH(x) && checkV(y)) {
        bitmap_.pixels()[y * bitmap_.width() + x] = color;
    }
}

} // namespace priv
} // namespace odtr
