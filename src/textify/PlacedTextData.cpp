
#include "PlacedTextData.h"

namespace textify {
namespace priv {

PlacedTextData::PlacedTextData(PlacedGlyphs &&glyphs_,
                               PlacedDecorations &&decorations_,
                               const compat::FRectangle &textBounds_) :
    glyphs(std::move(glyphs_)),
    decorations(std::move(decorations_)),
    textBounds(textBounds_) {
}

} // namespace priv
} // namespace textify
