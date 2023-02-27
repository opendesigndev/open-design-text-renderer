
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

#include "textify-api.h"

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
    /// Glyph index. TODO: Make sure the index corresponds to index in text and not in the glyphs.
    size_t index = 0;
};
using PlacedGlyphPtr = std::unique_ptr<PlacedGlyph>;
using PlacedGlyphs = std::vector<PlacedGlyphPtr>;
using PlacedGlyphsPerFont = std::map<FontSpecifier, PlacedGlyphs>;

} // namespace odtr
