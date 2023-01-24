
#include "PlacedTextData.h"

namespace textify {
namespace priv {

PlacedTextData::PlacedTextData(const PlacedGlyphs &glyphs_,
                               const PlacedDecorations &decorations_,
                               const compat::FRectangle &textBounds_) :
    glyphs(glyphs_),
    decorations(decorations_),
    textBounds(textBounds_) {
}

} // namespace priv
} // namespace textify
