
#include "PlacedTextData.h"

namespace textify {
namespace priv {

PlacedTextData::PlacedTextData(PlacedGlyphsPerFont &&glyphs_,
                               PlacedDecorations &&decorations_,
                               const compat::FRectangle &textBounds_,
                               float firstBaseline_) :
    glyphs(std::move(glyphs_)),
    decorations(std::move(decorations_)),
    textBounds(textBounds_),
    firstBaseline(firstBaseline_) {
}

} // namespace priv
} // namespace textify
