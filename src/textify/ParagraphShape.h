#pragma once

#include "Context.h"
#include "FormattedParagraph.h"
#include "LineBreaker.h"
#include "TypesetJournal.h"
#include "reported-fonts-utils.h"

#include "fonts/FaceTable.h"

#include <unordered_set>


namespace textify {
namespace utils {
class Log;
}
}

namespace textify {
namespace priv {

/** A sequence of GlyphShape objects, which forms the visual representation of a paragraph.
 *
 * To be converted using Harfbuzz from FormattedParagraph.
 */
class ParagraphShape
{
public:
    struct ParagraphMetrics
    {
        typedef int metric;

        metric lineWidth;
        metric maxAscender;
        metric minDescender;
        spacing lineHeight;
        spacing naturalLineHeight;
    };

    struct DrawResult
    {
        TypesetJournal journal;

        /**
         * @brief Size of maximum descender occuring on the last line of the
         * paragraph. Is a non-positive number.
         */
        spacing lastlineDescender = 0;

        /**
         * @brief  Size of maximum ascender on the first line of the paragraph.
         */
        spacing firstAscender = 0;

        /**
         * @brief  Size of maximum descender on the first line of the paragraph.
         */
        spacing firstDescender = 0;

        /**
         * @brief Height of the first line.
         */
        spacing firstLineHeight = 0;

        /**
         * @brief Default height of the first line.
         */
        spacing firstLineDefaultHeight = 0;

        /**
         * @brief Maximum bearing of any glyph in the first line.
         */
        float firstLineBearing = 0;

        /**
         * @brief Value of maximum line width in the paragraph.
         */
        float maxLineWidth = 0;

        /**
         * @brief Value of the leftmost position of a line within the paragraph.
         *
         * @note Works with RTL as well.
         */
        float leftmost = 0.0f;

        /**
         * @brief Value of the leftmost position of the first line of the paragraph.
         *
         * @note Should work with RTL as well (not tested).
         */
        float leftFirst = 0.0f;
    };
    using DrawResults = std::vector<DrawResult>;

    struct ShapeResult
    {
        /* implicit */ ShapeResult(bool success);

        void insertFontItem(const std::string& faceID, bool fallback);

        FlagsTable fallbacks;

        bool success;
    };

    class ReportedFaces : public std::unordered_set<ReportedFontItem>
    {
    public:
        void merge(const ReportedFaces& other);
    };

    ParagraphShape(const utils::Log& log, const FaceTable &faceTable);

    ParagraphShape(const ParagraphShape&) = delete;
    ParagraphShape& operator=(const ParagraphShape&) = delete;

    /// Initialize with Glyphs and Lines - skips the shaping phase.
    void initialize(const GlyphShapes &glyphs,
                    const LineSpans &lineSpans);

    /**
     * Transform @a paragraph into text shapes -- ie join characters into ligatures etc.
     *
     * @param paragraph Formatted paragraph to be transformed
     * @param width     Text width used for line breaking.
     */
    ShapeResult shape(const FormattedParagraph& paragraph,
                      float width,
                      bool loadGlyphsBearings);

    /**
     * Transform the shape into glyphs (images).
     *
     * @param[in] ctx              Context with configuration, fonts etc.
     * @param[in] left             Horizontal offset
     * @param[in] width            Text width used for line breaking.
     * @param[inout] y             Vertical offset. Use output for the following paragraph (if any)
     * @param[inout] positioning   Vertical positioning. Use output for the following paragraph (if any)
     * @param[in] scale            Scaling factor
     * @param[last] last           Drawing last paragraph
     */
    DrawResult draw(const Context& ctx,
                    int left,
                    int width,
                    float& y,
                    VerticalPositioning& positioning,
                    float scale,
                    bool last,
                    bool alphaMask) const;

    HorizontalAlign firstLineHorizontalAlignment() const;

    /**
     * Returns true if a line height is explicitly set, otherwise (when it's
     * derived from a font size) returns false.
     **/
    bool hasExplicitLineHeight() const;

    /// Read-only access to the specified glyph.
    const GlyphShape &glyph(std::size_t index) const;
    /// Read-only access to glyphs.
    const std::vector<GlyphShape> &glyphs() const;
    /// Read-only access to line spans.
    const LineSpans& lineSpans() const;

private:
    /// Get caret position at the start of a line
    void startCaret(const LineSpan& lineSpan, float& y, VerticalPositioning& positioning, float scale) const;
    spacing evalLineHeight(const uint32_t codepoint, const ImmediateFormat& fmt, const FacePtr face) const;

    // return {spaceCoef, nonSpaceCoef, lineWidth};
    struct JustifyResult
    {
        float spaceCoef;
        float nonSpaceCoef;
        float lineWidth;

        bool success;
    };

    struct JustifyParams
    {
        float width;                 //!<  width of the text field
        bool justifyAmbiguous;       //!< justify ambiguous lines, such as those ending with a soft break
        bool limitJustifySpaceWidth; //!< limit interlexical space width during justification
    };

    /** Evaluates text justification.
     *
     * \param[in]   lineSpan            The line span to be justified
     * \param[in]   params              Justify process configuration.
     *
     * \returns     Coefficient with which to multiply space width, coefficient with which to multiply non space width
     * and line width
     */
    JustifyResult justify(const LineSpan& lineSpan, const JustifyParams &params) const;

    /** Evaluates kerning
     *
     * Evaluates kerning between the glyph and index idx and the previous one.
     * Does not check whether the glyph at idx is the first one or not.
     */
    float kern(const FacePtr face, const int idx, float scale) const;

    /**
     * Gets the lowest descender in a given line.
     *
     * @param lineSpan      examined line
     * @param scale         scaling factor
     *
     * @return  Value of the most extreme descender in the line (descenders are
     *          negetive integers, so lower the value the bigger the descender).
     */
    spacing maxDescender(const LineSpan& lineSpan, float scale) const;

    /**
     * Gets the highest ascender in a given line.
     *
     * @param lineSpan      examined line
     * @param scale         scaling factor
     *
     * @return  Value of the most extreme ascender in the line.
     */
    spacing maxAscender(const LineSpan& lineSpan, float scale) const;

    /**
     * Gets the height of a given line.
     *
     * @param lineSpan      examined line
     * @param scale         scaling factor
     *
     * @return  Line height.
     */
    spacing maxLineHeight(const LineSpan& lineSpan, float scale) const;

    /**
     * Gets the default height of a given line.
     *
     * @param lineSpan      examined line
     * @param scale         scaling factor
     *
     * @return  Default lune height.
     */
    spacing maxLineDefaultHeight(const LineSpan& lineSpan, float scale) const;

    /**
     * Gets the largest Y-axis bearing in a line.
     *
     * @param lineSpan      examined line
     * @param scale         scaling factor
     *
     * @return  Default line height.
     */
    spacing maxBearing(const LineSpan& lineSpan, float scale) const;

    /**
     * Evaluate and setup Harfbuzz shaping features.
     *
     * Concerns ligatures, kerning, etc.
     */
    std::vector<hb_feature_t> setupFeatures(const ImmediateFormat& format) const;

    bool validateUserFeatures(const FacePtr face, const TypeFeatures& features) const;

    /// A consequent sequence of Glyph Shapes with the same format.
    struct Sequence
    {
        int start, len;
        ImmediateFormat format;
    };

    /**
     * Shape a sequence of characters with uniform format within a paragraph.
     *
     * @param[in]  seq          Sequence to be shaped
     * @param[in]  paragraph    Paragraph providing characters
     * @param[out] result       Shape result can transfer used fonts.
     */
    void shapeSequence(const Sequence& seq,
                       const FormattedParagraph& paragraph,
                       const FaceTable& faces,
                       const LineBreaker::LineStarts &lineStartsOpt,
                       bool loadGlyphsBearings,
                       ShapeResult& result);

private:
    /// Output of the shaping phase and input to the drawing phase.
    struct ShapingPhaseOutput {
        GlyphShapes glyphs_;
        LineSpans lineSpans_;
    } shapingPhaseOutput_;

    // TODO: Matus: Whom are these font faces reported to? It seems like nobody reads this.
    ReportedFaces reportedFaces_;

    const utils::Log &log_;
    const FaceTable &faceTable_;
};
using ParagraphShapePtr = std::unique_ptr<ParagraphShape>;
using ParagraphShapes = std::vector<ParagraphShapePtr>;

} // namespace priv
} // namespace textify
