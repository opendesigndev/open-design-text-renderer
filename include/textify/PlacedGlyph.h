
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "textify-api.h"

namespace textify {

struct FontSpecifier {
    /// Font face Id.
    std::string faceId;

    /// Comparison operator - used to make this struct a map key.
    bool operator<(const FontSpecifier& other) const {
        return faceId < other.faceId;
    }
};

/// A single glyph which is a result of the shaping phase, placed into its containing layer.
struct PlacedGlyph {
    /// Font size.
    float fontSize = 0.0f;
    /// Glyph codepoint - index within the loaded font file.
    uint32_t glyphCodepoint;
    /// Glyph placement, specified by its four quad corners.
    struct QuadCorners {
        Vector2f topLeft;
        Vector2f topRight;
        Vector2f bottomLeft;
        Vector2f bottomRight;
    } placement;
    /// Glyph color (incl. alpha).
    uint32_t color;
};
using PlacedGlyphPtr = std::unique_ptr<PlacedGlyph>;
using PlacedGlyphs = std::vector<PlacedGlyphPtr>;
using PlacedGlyphsPerFont = std::map<FontSpecifier, PlacedGlyphs>;

} // namespace textify
