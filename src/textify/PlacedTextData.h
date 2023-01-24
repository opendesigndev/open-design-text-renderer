
#pragma once

#include <memory>

#include <textify/PlacedGlyph.h>
#include <textify/PlacedDecoration.h>

#include "base-types.h"

namespace textify {
namespace priv {

struct PlacedTextData
{
    PlacedTextData() = default;
    PlacedTextData(const PlacedGlyphs &glyphs_,
                   const PlacedDecorations &decorations_,
                   const compat::FRectangle &textBounds_);

    PlacedGlyphs glyphs;
    PlacedDecorations decorations;
    // TODO: Matus: Remove
    compat::FRectangle textBounds;
};
using PlacedTextDataPtr = std::unique_ptr<PlacedTextData>;

} // namespace priv
} // namespace textify
