
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace textify {

struct Vector2f {
    float x, y;
};

struct PlacedGlyph {
    // TODO: Matus: This font face Id should be moved to some other place
    //   Maybe the glyphs should be groupped by face Ids
    std::string fontFaceId;
    /// Font size
    float fontSize = 0.0f;
    /// Glyph codepoint - index within the loaded font file
    uint32_t glyphCodepoint;
    /// Glyph placement, specified by its four quad corners.
    struct QuadCorners {
        Vector2f topLeft;
        Vector2f topRight;
        Vector2f bottomLeft;
        Vector2f bottomRight;
    } placement;
    /// Glyph color (incl. alpha)
    uint32_t color;
};
using PlacedGlyphPtr = std::unique_ptr<PlacedGlyph>;
using PlacedGlyphs = std::vector<PlacedGlyphPtr>;

} // namespace textify
