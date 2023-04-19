#pragma once

#include "Glyph.h"
#include "base.h"

#include "../common/result.hpp"

namespace odtr {

struct ScaleParams
{
    RenderScale vectorScale;
    RenderScale bitmapScale;
};

struct Decompositor
{
    // REFACTOR
    // Path::Parts parts;
    compat::Vector2f currentPoint;
};

/**
 * @brief Manages glyph retrieval, transformation and rendering.
 */
class GlyphAcquisitor
{
public:
    struct Parameters
    {
        bool isColor = false;
        bool scalable = false;

        /** The face is a color font, but CPAL table is not yet supported.
         *
         *  CPAL (Color Palette Table) defines colors used by multiple
         *  vector outlines defined in COLR (Color Table).
         *
         *  Remove this when Freetype renders color outlines (maybe 2.10).
         */
        bool cpalUsed = false;
        FT_Int32 loadflags = FT_LOAD_DEFAULT;
    };

    GlyphAcquisitor();

    /**
     * @brief Call this before calling acquire() to ensure correct retrieval of glyph.
     *
     * @param params    Retrieve from Face
     * @param ftFace    Retrieve from Face
     */
    void setup(const Parameters& params, FT_Face ftFace);

    /**
     * @brief See Face::acquireGlyph() for info.
     */
    GlyphPtr acquire(FT_UInt codepoint, const compat::Vector2f& offset, const ScaleParams& scale, bool render) const;

private:
    /**
     * @brief Setup and retrieve glyph slot with loaded glyph from face. For more info see Face::acquireGlyph().
     */

    Result<FT_GlyphSlot,bool> acquireSlot(FT_UInt codepoint, const compat::Vector2f& offset, RenderScale scale) const;
    /**
     * @brief Translate outline from glyph slot to a Path
     *
     * @param glyphSlot Slot in face containing glyph info
     *
     * @return Path ready to be scaled as vector shape or serialized to Octopus
     */

    // REFACTOR
    // Path acquireOutline(const FT_GlyphSlot& glyphSlot) const;

    /**
     * @brief Create either color or grayscale glyph
     *
     * @param glyphSlot Slot in face containing glyph info
     *
     * @return Glyph structure to be filled with data
     */

    GlyphPtr createGlyph(FT_GlyphSlot glyphSlot) const;


    // REFACTOR
    // const FT_Outline_Funcs funcs_;
    Parameters params_;
    FT_Face ftFace_ = nullptr;
};

} // namespace odtr
