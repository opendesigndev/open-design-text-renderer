
#pragma once

#include "text-format.h"
#include "VisualRun.h"

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
    VisualRuns visualRuns;
    float lineWidth;
    TextDirection baseDir;
    Justifiable justifiable;
};
using LineSpans = std::vector<LineSpan>;

} // namespace priv
} // namespace textify
