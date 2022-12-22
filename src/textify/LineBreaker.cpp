#include "LineBreaker.h"

#include "tabstops.h"

#include "utils/Log.h"

#include <unicode/brkiter.h>

namespace textify {
namespace priv {

LineBreaker::LineBreaker(const utils::Log& log,
                         const std::vector<GlyphShape>& glyphs,
                         const std::vector<VisualRun>& visualRuns,
                         const TextDirection dir)
    : log_(log), glyphs_(glyphs), visualRuns_(visualRuns), baseDir_(dir)
{
}

bool LineBreaker::isSoftBreak(compat::qchar c)
{
    // Line separator End of text (deprecated)
    return c == 0x2028 || c == 0x0003;
}

LineBreaker::LineStarts LineBreaker::analyzeBreaks(const utils::Log& log, const std::vector<compat::qchar>& text)
{
    std::vector<bool> opportunities;

    auto locale = icu::Locale::getDefault(); // TODO Recognize locale from text
    UErrorCode error = U_ZERO_ERROR;
    auto bi = icu::BreakIterator::createLineInstance(locale, error);
    auto utext = ustring::fromUTF32(reinterpret_cast<const UChar32*>(text.data()), static_cast<int32_t>(text.size()));
    opportunities.assign(utext.length() + 1, false); // The last possible line start is just beyond the last character

    if (U_FAILURE(error)) {
        log.error("Cannot instantiate ICU line break iterator: {}.", u_errorName(error));
        delete bi;
        return opportunities;
    }

    bi->setText(utext);
    do {
        opportunities.at(bi->current()) = true;
    } while (bi->next() != icu::BreakIterator::DONE);

    delete bi;
    return opportunities;
}

void LineBreaker::breakLines(int maxLineWidth)
{
    if (glyphs_.empty()) {
        return;
    }

    lineSpans_.clear();
    Context ctx;
    auto insertLine = [&](long startIndex, long endIndex, float lineWidth, LineSpan::Justifiable justifiable) {
        if (startIndex != endIndex) {
            lineSpans_.push_back({startIndex, endIndex, lineWidth, baseDir_, justifiable});
            ctx.resetLine();
            auto nextlineOffset = newlineOffset(glyphs_[endIndex-1].format.tabStops);
            ctx.spaceWidth = nextlineOffset.value_or(0.0f);

            return true;
        }
        return false;
    };

    if (!glyphs_[0].lineStart.has_value()) {
        log_.warn("Line breaking opportunities not present.");
        for (unsigned long i = 0ul; i < glyphs_.size(); ++i)
            advanceContext(i, ctx);
        insertLine(0, static_cast<long>(glyphs_.size()), ctx.wordWidth + ctx.spaceWidth, LineSpan::Justifiable::POSITIVE);
        return;
    }

    for (unsigned long i = 0ul; i < glyphs_.size(); ++i) {
        if (isSoftBreak(glyphs_[i].character)) {
            if (ctx.lineStart == i) { // Blank line containing only a soft break
                insertLine(ctx.lineStart, i + 1, 0, LineSpan::Justifiable::NEGATIVE);
            } else {
                insertLine(ctx.lineStart, i, ctx.lineWidth + ctx.wordWidth + ctx.spaceWidth, LineSpan::Justifiable::DOCUMENT);

                // Blank line at the end of a paragraph
                if (i == glyphs_.size() - 1) {
                    insertLine(i, i + 1, 0, LineSpan::Justifiable::NEGATIVE);
                }
            }
            ctx.lineStart = static_cast<int>(i + 1);
            continue;
        }

        if (i > 0 && glyphs_[i].lineStart.value()) {
            ctx.lineWidth += ctx.wordWidth + ctx.spaceWidth;
            ctx.wordWidth = ctx.spaceWidth = 0;
            ctx.lineEnd = static_cast<int>(i);
        }

        advanceContext(i, ctx);

        if (maxLineWidth > 0 && std::floor(ctx.lineWidth + ctx.wordWidth) > maxLineWidth) {
            if (!insertLine(ctx.lineStart, ctx.lineEnd, ctx.lineWidth, LineSpan::Justifiable::POSITIVE)) {
                continue;
            }

            i = ctx.lineStart = ctx.lineEnd;
            advanceContext(i, ctx);
        }
    }
    // insert last line
    insertLine(ctx.lineStart, static_cast<int>(glyphs_.size()), ctx.lineWidth + ctx.wordWidth + ctx.spaceWidth, LineSpan::Justifiable::NEGATIVE);

    performBidi();
    removeTrailingSpaces();
}

void LineBreaker::advanceContext(long i, Context& ctx)
{
    const spacing advance = glyphs_[i].horizontalAdvance + glyphs_[i].format.letterSpacing;

    if (isWhitespace(glyphs_[i].character)) {
        if (isTabStop(glyphs_[i].character)) {
            auto caret = ctx.spaceWidth + ctx.wordWidth + ctx.lineWidth;
            auto tabstopIt = glyphs_[i].format.tabStops.upper_bound(caret);
            if (tabstopIt != end(glyphs_[i].format.tabStops)) {
                auto tabstop = *tabstopIt;
                ctx.lineWidth = tabstop;
                ctx.spaceWidth = 0.0;
                ctx.wordWidth = 0.0;
            }
        } else {
            ctx.spaceWidth += advance;
        }
    } else {
        ctx.wordWidth += advance;
    }

    // Use this if lettter spacing should not be added to the last letter of a word.
    // It is commented out because I found no recent evidence for its use.

    //auto advance = glyphs_[i].horizontalAdvance;

    //if (isWhitespace(glyphs_[i].character)) {
    //    ctx.spaceWidth += advance + glyphs_[i].format.letterSpacing;
    //} else {
    //    if (i + 1 < glyphs_.size() && !isWhitespace(glyphs_[i + 1].character)) {
    //        // ads letter spacing unless the letter is the last one within a word
    //        advance += glyphs_[i].format.letterSpacing;
    //    }
    //    ctx.wordWidth += advance;
    //}
}

void LineBreaker::removeTrailingSpaces()
{
    for (LineSpan &lineSpan : lineSpans_) {
        for (long i = lineSpan.end - 1; i >= lineSpan.start; --i) {
            if (isWhitespace(glyphs_[i].character)) {
                lineSpan.lineWidth -= glyphs_[i].horizontalAdvance + glyphs_[i].format.letterSpacing;
            } else {
                lineSpan.end = i + 1;
                break;
            }
        }
    }
}

bool LineBreaker::performBidi()
{
    const auto sumWidths = [&](long start, long end) {
        Context ctx;
        for (long j = start; j < end; ++j) {
            advanceContext(j, ctx);
        }

        return ctx.wordWidth + ctx.spaceWidth;
    };

    for (const auto& run : visualRuns_) {
        for (int i = 0; i < lineSpans_.size(); ++i) {
            // Check for intersection of line span and run
            if (run.start >= lineSpans_[i].start && run.start < lineSpans_[i].end) {
                if (run.end <= lineSpans_[i].end) { // run fits line span
                    lineSpans_[i].visualRuns.push_back({run.start,
                                                        run.end,
                                                        run.dir,
                                                        sumWidths(run.start, run.end)});
                    break;

                } else { // run spreads across multiple line spans
                    lineSpans_[i].visualRuns.push_back({run.start,
                                                        lineSpans_[i].end,
                                                        run.dir,
                                                        sumWidths(run.start, lineSpans_[i].end)});

                    for (auto j = i + 1; j < lineSpans_.size(); ++j) {
                        if (run.end <= lineSpans_[j].end) { // run ends here
                            lineSpans_[j].visualRuns.push_back({lineSpans_[j].start,
                                                                run.end,
                                                                run.dir,
                                                                sumWidths(lineSpans_[j - 1].end, run.end)});
                            break;
                        }
                        else { // run continues
                            lineSpans_[j].visualRuns.push_back({lineSpans_[j].start,
                                                                lineSpans_[j].end,
                                                                run.dir,
                                                                sumWidths(lineSpans_[j - 1].end, lineSpans_[j].end)});
                        }
                    }
                    break;
                }
            }
        }
    }

    return true;
}

} // namespace priv
} // namespace textify
