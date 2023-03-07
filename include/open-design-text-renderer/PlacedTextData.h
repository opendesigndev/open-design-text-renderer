
#pragma once

#include <open-design-text-renderer/PlacedGlyph.h>
#include <open-design-text-renderer/PlacedDecoration.h>

namespace odtr {

/// The result of the shaping phase (glyphs and decorations), placed into their containing layer.
struct PlacedTextData
{
    PlacedTextData() = default;
    PlacedTextData(PlacedGlyphsPerFont &&glyphs_,
                   PlacedDecorations &&decorations_,
                   const FRectangle &textBounds_,
                   const Matrix3f &transform_);

    /// Glyphs and their placements.
    PlacedGlyphsPerFont glyphs;
    /// Decorations and their placements.
    PlacedDecorations decorations;
    /// Text bounds within the layer.
    FRectangle textBounds;
    /// Text transfomation within the layer.
    Matrix3f textTransform;
};
using PlacedTextDataPtr = std::unique_ptr<PlacedTextData>;

} // namespace odtr
