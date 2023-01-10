#pragma once

#include "base.h"
#include "FreetypeHandle.h"
#include "Glyph.h"
#include "GlyphAcquisitor.h"

#include "otf/otf.h"

#include "common/result.hpp"

#include <string>

namespace textify {

/**
 * This class represents a typeface (corresponding to one TTF/OTF file).
 * Internally, it holds a FreeType FT_Face handle, and is necessary to get glyphs.
 */
class Face
{
public:
    Face(FT_Library ftLibrary, const char* filename, FT_Long faceIndex);
    Face(FT_Library ftLibrary, const compat::byte* fileBytes, int length, FT_Long faceIndex);
    Face(const Face&) = delete;
    ~Face();
    Face& operator=(const Face&) = delete;

    bool ready() const;
    void logFeatures(bool all = false) const;
    bool isColorFont(bool log = false) const;
    bool isScalable() const { return params_.scalable; }
    /**
     * Scale by a 16.16 factor to 26.6 fractional pixels and convert to
     * floating point pixels.
     */
    float scaleFontUnits(int fontParam, bool y_scale = false) const;

    const FT_Face& getFtFace() const { return ftFace_; }
    hb_font_t* getHbFont() const;

    // TODO: Matus: Temporary? - copy of setSize, without actually mofifying anything.
    Result<font_size,bool> getBestSizeToSet(font_size size) const;

    Result<font_size, bool> setSize(font_size size);
    void setFlags(FT_Int32 loadflags) const { params_.loadflags = loadflags; }

    const std::string& getPostScriptName() const;

    /**
     * @brief One call to (acquire glyphs to) rule them all.
     *
     * @param codepoint       Index of glyph in the face
     * @param xOffset         Sub-pixel offset of glyph, should be <0, 1>
     * @param scale           Render scale
     * @param render          Whether to render a scalable glyph to bitmap. Not applicable to bitmap fonts.
     *
     * @return              Glyph ready to be drawn onto bitmap or scaled as vector shape.
     */
    GlyphPtr acquireGlyph(FT_UInt codepoint,
                          const compat::Vector2f& offset,
                          const ScaleParams& scale,
                          bool render = true,
                          bool disableHinting = false) const;

    /**
     * Get glyph advance right from the face.
     */
    FT_Fixed getGlyphAdvance(hb_codepoint_t codepoint) const;

    bool hasGlyph(compat::qchar cp) const;

    bool hasOpenTypeFeature(const std::string& featureTag) const;

private:
    void initialize();

    void recreateHBFont();

    FT_Face ftFace_;
    hb_font_t* hbFont_;

    std::string postscriptName_;

    mutable GlyphAcquisitor::Parameters params_;
    mutable GlyphAcquisitor acquisitor_;

    otf::Features features_;
};

/**
 * This class holds a pointer to Face, without memory management.
 * destroy must be called on the original instance in order to free the Face.
 * This class exists because of Emscripten bindings, which contain regular pointers.
 * For Emscripten, it provides a constructor which accepts the bytes of a font file to load.
 * Otherwise, it can be implicitly constructed from a Face pointer.
 *
 * @deprecated  Face is not used in emscripten directly.
 */
class FacePtr
{

public:
    FacePtr();
    FacePtr(Face* face);
    void destroy();
    Face* operator->() const;
    operator Face*() const;

private:
    Face* face_;
};

} // namespace textify
