
#include "compat/arithmetics.hpp"
#include "pixel-conversion.h"

#include <cmath>
#include <algorithm>

namespace textify {
namespace compat {

float cComp_b2f(byte v) {
    return 1.f/255.f*float(v);
}

byte cComp_f2b(float v) {
    return (byte) clamp(int(256.f*v), 0x00, 0xff);
}

Color pixelToColor(Pixel32 p) {
    return Color {
        cComp_b2f(byte(p)),
        cComp_b2f(byte(p>>8)),
        cComp_b2f(byte(p>>16)),
        cComp_b2f(byte(p>>24))
    };
}

Pixel32 colorToPixel(Color p) {
    return
        (uint32_t) cComp_f2b(p.r)|
        (uint32_t) cComp_f2b(p.g)<<8|
        (uint32_t) cComp_f2b(p.b)<<16|
        (uint32_t) cComp_f2b(p.a)<<24;
}

Pixel32 grayToPixel(byte v)
{
    return (uint32_t) v | (uint32_t) v << 8 | (uint32_t) v << 16| (uint32_t) v << 24;
}

Pixel32 premultiplyPixel(Pixel32 p) {
    Pixel32 a = p>>24&0xffu;
    return
        (a*(p     &0xffu)+0xffu)>>8            |
        (a*(p>>  8&0xffu)+0xffu    &0x0000ff00u)|
        ((a*(p>>16&0xffu)+0xffu)<<8&0x00ff0000u)|
        a<<24;
}

Pixel32 unpremultiplyPixel(Pixel32 p) {
    if (float a = cComp_b2f((byte) (p>>24))) {
        float r = cComp_b2f((byte)  p     )/a;
        float g = cComp_b2f((byte) (p>>8 ))/a;
        float b = cComp_b2f((byte) (p>>16))/a;
        return
            (Pixel32) cComp_f2b(r)|
            (Pixel32) cComp_f2b(g)<<8|
            (Pixel32) cComp_f2b(b)<<16|
            (p&0xff000000u);
    }
    return Pixel32();
}

} // namespace compat
} // namespace textify
