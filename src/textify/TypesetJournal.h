#pragma once

#include "base.h"
#include "Face.h"

#include "compat/basic-types.h"
#include "common/result.hpp"

#include <vector>

namespace textify {
namespace priv {

class TypesetJournal
{
public:
    struct LowHighPair
    {
        int high;
        int low;

        /**
         * Detects when a fraction of the range overflows given limit.
         *
         * @return  true if @a limit is strictly in between of @a high and @low
         */
        bool partialOverflow(int limit) const;
    };

    /// Specifies a decoration for a range of pixels
    struct DecorationRecord
    {
        struct
        {
            int first, last;
        } range; ///< horizontal range in pixels

        Decoration type;
        Pixel32 color;
        int offset; ///< vertical distance from the top
        float thickness;

        LowHighPair indices { 0, 0 };
    };

    struct DecorationInput
    {
        IPoint2 start;
        IPoint2 end;

        Decoration type;
        Pixel32 color;

        const Face* face;
    };

    struct LineRecord
    {
        int lowPoint() const;

        /**
         * Returns the highest and the lowest point of the glyphs within a line.
         *
         * @return      pair of high and low point
         */
        LowHighPair glyphBitmapExtremes() const;

        std::vector<GlyphPtr> glyphJournal_;
        std::vector<DecorationRecord> decorationJournal_;

        // TODO: Matus: offsets
        std::vector<compat::Vector2f> offsets_;
    };

    struct DrawResultData
    {
        std::size_t numGlyphsRendered;
    };

    using DrawResult = Result<DrawResultData,bool>;

    TypesetJournal() : lastLinePolicy_(LastLinePolicy::CUT) {}

    void setLastLinePolicy(LastLinePolicy policy) { lastLinePolicy_ = policy; }

    void startLine(float y);
    void addGlyph(GlyphPtr glyph, const compat::Vector2f &offset);

    /**
     * Add decoration, extend the last one if possible.
     */
    void addDecoration(const DecorationInput& d, float scale, int index);

    /**
     * Draw glyphs and decoration stored within jounal to the given bitmap.
     */
    DrawResult draw(compat::BitmapRGBA& bitmap, const compat::Rectangle& bounds, int textHeight, const compat::Rectangle& viewArea, const compat::Vector2i& offset) const;


    size_t size() const { return lineJournal_.size(); }
    const auto& getLines() const { return lineJournal_; }

    compat::Rectangle stretchedBounds(int yOffset, int yMax) const;

private:
    /**
     * Extends range of the last decoration if it shares the @a type.
     **/
    bool extendLastDecoration(Decoration type, int newRange, int newIndex);

    /**
     * Renders glyphs from lines journal to the given bitmap.
     *
     * @param[out] bitmap        destination bitmap
     * @param[in]  textHeight    scaled height of the original text layer
     * @param[in]  viewArea      actually visible area
     * @param[in]  offset        glyphs offset relative to @a bitmap
     */
    DrawResult drawGlyphs(compat::BitmapRGBA& bitmap, const compat::Rectangle& bounds, int textHeight, const compat::Rectangle& viewArea, const compat::Vector2i& offset) const;

    /**
     * Renders decorations from decorartions journal to the given bitmap.
     *
     * @param bitmap    destination bitmap
     * @param offset    offset relative a @a bitmap
     *
     * @todo    Improve rendering of scaled decorations (esp. double strikes when zoomed out).
     */
    void drawDecorations(compat::BitmapRGBA& bitmap, const compat::Vector2i& offset) const;

    /// Lines
    std::vector<LineRecord> lineJournal_;

    LastLinePolicy lastLinePolicy_;
};

} // namespace priv
} // namespace textify
