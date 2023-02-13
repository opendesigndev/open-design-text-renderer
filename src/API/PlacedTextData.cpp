
#include <textify/PlacedTextData.h>

namespace textify {

PlacedTextData::PlacedTextData(PlacedGlyphsPerFont &&glyphs_,
                               PlacedDecorations &&decorations_,
                               const FRectangle &textBounds_,
                               std::optional<float> baseline_) :
    glyphs(std::move(glyphs_)),
    decorations(std::move(decorations_)),
    textBounds(textBounds_),
    baseline(baseline_) {
}

} // namespace textify
