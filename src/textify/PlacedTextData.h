
#pragma once

#include <memory>

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
                   const compat::FRectangle &textBounds_);

    PlacedGlyphsPerFont glyphs;
    PlacedDecorations decorations;
    // TODO: Matus: Remove
    compat::FRectangle textBounds;
};
using PlacedTextDataPtr = std::unique_ptr<PlacedTextData>;
using PlacedTextResult = Result<PlacedTextDataPtr, TextShapeError>;

} // namespace priv
} // namespace textify
