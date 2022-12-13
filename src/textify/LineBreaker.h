#pragma once

#include "GlyphShape.h"
#include "base.h"
#include "text-format.h"

#include <optional>
#include <vector>

// Forward declaration
namespace textify {
namespace utils {
class Log;
}
}

namespace textify {
namespace priv {

struct LineSpan
{
    enum class Justifiable
    {
        POSITIVE, //!< This line can be justified.
        NEGATIVE, //!< This line should not be justified. Example: The last line in a paragraph.
        DOCUMENT  //!< Whether this line can be justified or not depends on the document origin. Example: A line
                  //!< ending with a soft break (0x2028).
    };

    LineSpan() = default;
    LineSpan(long start,
             long end,
             float lineWidth,
             TextDirection dir = TextDirection::LEFT_TO_RIGHT,
             Justifiable justifiable = Justifiable::POSITIVE);

    //! Glyph indices in left-closed interval [start, end)
    long start, end;

    //! BiDi visual runs.
    std::vector<VisualRun> visualRuns;
    float lineWidth;
    TextDirection baseDir;
    Justifiable justifiable;
};
using LineSpans = std::vector<LineSpan>;


class LineBreaker
{
public:
    typedef std::optional<std::vector<bool>> LineStarts;

    LineBreaker(const utils::Log& log,
                const std::vector<GlyphShape>& glyphs,
                const std::vector<VisualRun>& visualRuns,
                const TextDirection dir);

    static bool isSoftBreak(compat::qchar c);

    /**
     *  Find break opportunities as specified by the Unicode Line Breaking Algorithm
     */
    static LineStarts analyzeBreaks(const utils::Log& log, const std::vector<compat::qchar>& text);

    /**
     *  Break lines according to previously found breaks.
     */
    void breakLines(int maxLineWidth);

    const LineSpans& getLines() const { return lineSpans_; }

    LineSpans&& moveLines() { return std::move(lineSpans_); }

private:
    struct Context
    {
        void resetLine()
        {
            lineWidth = 0;
            wordWidth = 0;
            spaceWidth = 0;
        }

        spacing lineWidth = 0;
        spacing wordWidth = 0;
        spacing spaceWidth = 0;
        int lineStart = 0;
        int lineEnd = 0;
    };

    /**
     *  Get advance of glyph at index i and store appropriately.
     */
    void advanceContext(long i, Context& ctx);

    /**
     *  Create tighter linespans for use in justification by omitting trailing spaces.
     */
    void removeTrailingSpaces();

    /**
     * Perform the Unicode Bidirectional Algorithm on line spans.
     *
     * @note This was previously done in FormattedParagraph leading to such issues as
     * incorrect shaping or RTL text going not just right to left, but also bottom up.
     */
    bool performBidi();

    const utils::Log& log_;

    const std::vector<GlyphShape>& glyphs_;
    const std::vector<VisualRun>& visualRuns_;
    const TextDirection baseDir_;
    std::vector<LineSpan> lineSpans_;
};

} // namespace priv
} // namespace textify
