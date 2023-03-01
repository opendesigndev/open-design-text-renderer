
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "text-renderer-api.h"

namespace odtr {

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
    uint32_t codepoint;
    /// Origin position of the glyph (on the line).
    Vector2f originPosition;
    /// Glyph color (incl. alpha).
    uint32_t color;
    /// Index of the glyph within the input text.
    size_t index = 0;
};
using PlacedGlyphs = std::vector<PlacedGlyph>;
using PlacedGlyphsPerFont = std::map<FontSpecifier, PlacedGlyphs>;

} // namespace odtr
