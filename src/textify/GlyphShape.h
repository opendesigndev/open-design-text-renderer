#pragma once

#include "base.h"

#include <optional>

namespace textify {
namespace priv {

/**
 * A set of all values necessary to draw a single glyph,
 * such as position, color, and the codepoint which must be rendered from the Face found within format.
 */
struct GlyphShape
{
    ImmediateFormat format;
    uint32_t codepoint;         //!< Face dependent
    compat::qchar character;    //!< Unicode
    Pixel32 color;      // TODO why is this here and also in format?
    TextDirection direction;
    std::optional<bool> lineStart;

    spacing defaultLineHeight;

    // scalable values
    spacing horizontalAdvance;
    float ascender;     //!< Up to 1px error, see FT reference
    float descender;    //!< Up to 1px error, see FT reference
    spacing bearingX;   //!< X-axis bearing of the glyph
    spacing bearingY;   //!< Y-axis bearing of the glyph
    spacing lineHeight; // TODO why is this here and also in format?

    /**
     * Adjusts metrics by a given factor.
     *
     * This should be applied to non-scalable fonts to correct values taken from descrete levels.
     */
    void applyResizeFactor(float resizeFactor);

    struct Scalable
    {
        spacing horizontalAdvance;
        float ascender;     //!< Up to 1px error, see FT reference
        float descender;    //!< Up to 1px error, see FT reference
        spacing lineHeight;
        font_size size;

        spacing letterSpacing;
        spacing paragraphSpacing;
    };

    Scalable scaledGlyphShape(float scale) const;
};
using GlyphShapes = std::vector<GlyphShape>;

} // namespace priv
} // namespace textify
