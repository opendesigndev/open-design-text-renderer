
#include <open-design-text-renderer/PlacedTextData.h>

namespace odtr {

PlacedTextData::PlacedTextData(PlacedGlyphsPerFont &&glyphs_,
                               PlacedDecorations &&decorations_,
                               const FRectangle &textBounds_,
                               const Matrix3f &textTransform_) :
    glyphs(std::move(glyphs_)),
    decorations(std::move(decorations_)),
    textBounds(textBounds_),
    textTransform(textTransform_) {
}

} // namespace odtr
