
#pragma once

#include "basic-types.h"

namespace textify {
namespace compat {

float cComp_b2f(byte v);
byte cComp_f2b(float v);

Color pixelToColor(Pixel32 p);
Pixel32 colorToPixel(Color p);
Pixel32 grayToPixel(byte v);

Pixel32 premultiplyPixel(Pixel32 p);
Pixel32 unpremultiplyPixel(Pixel32 p);

} // namespace compat
} // namespace textify
