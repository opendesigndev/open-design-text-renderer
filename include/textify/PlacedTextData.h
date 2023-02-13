
#pragma once

#include <optional>

#include <textify/PlacedGlyph.h>
#include <textify/PlacedDecoration.h>

namespace textify {

/// The result of the shaping phase (glyphs and decorations), placed into their containing layer.
struct PlacedTextData
{
    PlacedTextData() = default;
    PlacedTextData(PlacedGlyphsPerFont &&glyphs_,
                   PlacedDecorations &&decorations_,
                   const FRectangle &textBounds_,
                   std::optional<float> baseline_);

    /// Glyphs and their placements.
    PlacedGlyphsPerFont glyphs;
    /// Decorations and their placements.
    PlacedDecorations decorations;
    /// Text bounds within the layer.
    FRectangle textBounds;
    /// First line baseline - optionally used for text scaling, the text should be scaled so that the first baseline is preserved.
    std::optional<float> baseline = std::nullopt;
};
using PlacedTextDataPtr = std::unique_ptr<PlacedTextData>;

} // namespace textify
