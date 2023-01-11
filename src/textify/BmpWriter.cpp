
#include "BmpWriter.h"

namespace textify {
namespace priv {

BmpWriter::BmpWriter(compat::BitmapRGBA& bitmap)
    : bitmap_(bitmap)
{ }

bool BmpWriter::checkV(int y) {
    return y >= 0 && y < bitmap_.height();
}

bool BmpWriter::checkH(int x) {
    return x >= 0 && x < bitmap_.width();
}

void BmpWriter::write(int x, int y, compat::Pixel32 color) {
    if (checkV(y)) {
        bitmap_.pixels()[y * bitmap_.width() + x] = color;
    }
}

} // namespace priv
} // namespace textify
