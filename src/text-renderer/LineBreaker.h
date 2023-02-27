#pragma once

#include <optional>
#include <vector>

#include "base.h"
#include "text-format.h"
#include "GlyphShape.h"
#include "LineSpan.h"

// Forward declaration
namespace odtr {
namespace utils {
class Log;
}
}

namespace odtr {
namespace priv {

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
} // namespace odtr
