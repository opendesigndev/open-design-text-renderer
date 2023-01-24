
#pragma once

#include <string>
#include <vector>

namespace textify {

struct Vector2f {
    float x, y;
};

struct PlacedGlyph {
    /// Glyph codepoint - index within the loaded font file
    uint32_t glyphCodepoint;
    /// Glyph position, specified by its four quad corners.
    struct QuadCorners {
        Vector2f topLeft, topRight, bottomLeft, bottomRight;
    } quadCorners;
    /// Glyph color
    uint32_t color;
    /// Font size
    float fontSize = 0.0f;
    // TODO: Matus: This font face Id should be moved to some other place
    //   Maybe the glyphs should be groupped by face Ids
    std::string fontFaceId;
};
using PlacedGlyphs = std::vector<PlacedGlyph>;

} // namespace textify
