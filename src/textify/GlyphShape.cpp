#include "GlyphShape.h"

#include <cmath>

namespace odtr {
namespace priv {

void GlyphShape::applyResizeFactor(float resizeFactor)
{
    defaultLineHeight *= resizeFactor;
    horizontalAdvance *= resizeFactor;
    ascender *= resizeFactor;
    descender *= resizeFactor;
    lineHeight *= resizeFactor;
}

GlyphShape::Scalable GlyphShape::scaledGlyphShape(float scale) const
{
    return Scalable{
        horizontalAdvance * scale,
        ascender * scale,
        descender * scale,
        lineHeight * scale,
        static_cast<font_size>(std::ceil(format.size * scale)),
        format.letterSpacing * scale,
        format.paragraphSpacing * scale,
    };
}

} // namespace priv
} // namespace odtr
