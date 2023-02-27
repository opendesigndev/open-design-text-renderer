
#include <textify/PlacedTextData.h>

namespace odtr {

PlacedTextData::PlacedTextData(PlacedGlyphsPerFont &&glyphs_,
                               PlacedDecorations &&decorations_,
                               const FRectangle &textBounds_,
                               const Matrix3f &textTransform_,
                               float baseline_) :
    glyphs(std::move(glyphs_)),
    decorations(std::move(decorations_)),
    textBounds(textBounds_),
    textTransform(textTransform_),
    baseline(baseline_) {
}

} // namespace odtr
