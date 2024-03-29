
#pragma once

#include "Bitmap.hpp"

#ifndef __EMSCRIPTEN__

namespace odtr {
namespace compat {

bool detectPngFormat(const byte* data, size_t len);
BitmapRGBAPtr loadPngImage(FILE *file);
bool savePngImage(const BitmapRGBA& bitmap, const char* filename);

} // namespace compat
} // namespace odtr

#endif

