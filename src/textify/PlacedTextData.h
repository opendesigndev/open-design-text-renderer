
#pragma once

#include <memory>
#include <optional>

#include <textify/PlacedGlyph.h>
#include <textify/PlacedDecoration.h>

#include "common/result.hpp"

#include "base-types.h"
#include "errors.h"

namespace textify {
namespace priv {

/// The result of the shaping phase (glyphs and decorations), placed into their containing layer.
struct PlacedTextData
{
    PlacedTextData() = default;
    PlacedTextData(PlacedGlyphsPerFont &&glyphs_,
                   PlacedDecorations &&decorations_,
                   const compat::FRectangle &textBounds_,
                   std::optional<float> firstBaseline_);

    /// Glyphs and their placements.
    PlacedGlyphsPerFont glyphs;
    /// Decorations and their placements.
    PlacedDecorations decorations;
    /// Text bounds within the layer.
    compat::FRectangle textBounds;
    /// First line baseline - optionally used for text scaling, the text should be scaled so that the first baseline is preserved.
    std::optional<float> firstBaseline = std::nullopt;
};
using PlacedTextDataPtr = std::unique_ptr<PlacedTextData>;
using PlacedTextResult = Result<PlacedTextDataPtr, TextShapeError>;

} // namespace priv
} // namespace textify
