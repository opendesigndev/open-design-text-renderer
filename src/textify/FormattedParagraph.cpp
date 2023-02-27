
#include "FormattedParagraph.h"

#include "fonts/FaceTable.h"
#include "fonts/FontManager.h"
#include "utils/Log.h"
#include "unicode/EmojiTable.h"

#include <unicode/ubidi.h>
#include <unicode/uchar.h>

#include <algorithm>
#include <locale>

namespace odtr {
namespace priv {

using namespace compat;

int FormattedParagraph::extractParagraph(FormattedParagraph& output,
                                         const qchar* string,
                                         const ImmediateFormat* format,
                                         int limit)
{
    // 0x2029 is paragraph separator
    // \r and \n are deprecated and should not be present in Octopus
    const std::vector<qchar> breaks = {'\r', '\n', 0x2029};

    auto isBreak = [&](const qchar& c) { return std::find(breaks.begin(), breaks.end(), c) != breaks.end(); };

    int pos = 0;
    while (pos < limit && string[pos] != 0 && !isBreak(string[pos]))
        ++pos;

    if (pos != 0)
        output.append(string, format, pos);
    else
        output.append(0x2028, *format); // append a blank line

    return ++pos;
}

FormattedParagraph::FormattedParagraph(const utils::Log& log)
    : log_(log)
{ }

const qchar* FormattedParagraph::getText() const
{
    if (text_.empty())
        return nullptr;
    return &text_[0];
}

const ImmediateFormat* FormattedParagraph::getFormat() const
{
    if (format_.empty())
        return nullptr;
    return &format_[0];
}

int FormattedParagraph::getLength() const
{
    return (int)text_.size();
}

void FormattedParagraph::append(qchar character, const ImmediateFormat& format)
{
    text_.push_back(character);
    this->format_.push_back(format);
}

void FormattedParagraph::append(const qchar* characters, const ImmediateFormat* format, int len)
{
    text_.insert(text_.end(), characters, characters + len);
    this->format_.insert(this->format_.end(), format, format + len);
}

struct UBidiHandle
{
    /* implicit */ UBidiHandle(UBiDi* obj)
        : obj(obj)
    { }

    bool operator!() const { return obj == nullptr; }

    /* implicit */ operator UBiDi*() const { return obj; }

    ~UBidiHandle()
    {
        ubidi_close(obj);
    }

private:
    UBiDi* obj;
};

bool FormattedParagraph::analyzeBidi()
{
    UErrorCode errorCode = U_ZERO_ERROR;
    const ustring utext = ustring::fromUTF32(reinterpret_cast<const UChar32*>(text_.data()), (int) text_.size());
    const int32_t uLen = utext.length();
    const UBiDiDirection baseDir = ubidi_getBaseDirection(utext.getBuffer(), uLen);
    baseDirection_ = static_cast<TextDirection>(baseDir);
    UBidiHandle ubidi = ubidi_openSized(uLen, 0, &errorCode);

    if (!ubidi || U_FAILURE(errorCode)) {
        log_.warn("BiDi error: Cannot allocate structure.");
        return false;
    }
    ubidi_setPara(ubidi, utext.getBuffer(), uLen, /*baseDir*/ UBIDI_LTR, NULL, &errorCode);

    if (U_FAILURE(errorCode)) {
        log_.warn("BiDi error: Cannot perform reordering.");
        return false;
    }

    const int32_t nRuns = ubidi_countRuns(ubidi, &errorCode);
    if (U_FAILURE(errorCode)) {
        log_.warn("BiDi error: Cannot retrieve runs.");
        return false;
    }

    for (int32_t i = 0; i < nRuns; ++i) {
        int32_t start, length;
        // returns the directionality of the run, UBIDI_LTR==0 or UBIDI_RTL==1, never UBIDI_MIXED, never UBIDI_NEUTRAL
        // (see docs)
        const TextDirection dir = static_cast<TextDirection>(ubidi_getVisualRun(ubidi, i, &start, &length));
        visualRuns_.push_back({start, start + length, dir});
    }

    for (const VisualRun &run : visualRuns_) {
        // i iterates over UTF-16 code units, j iterates over UTF-32 code points
        const int32_t start = utext.getChar32Start(static_cast<int32_t>(run.start));
        for (int32_t i = start, j = start; i < run.end; i = utext.getChar32Limit(++i), ++j) {
            format_[j].direction = run.dir;
        }
    }

    return true;
}

void FormattedParagraph::applyFormatModifiers(FontManager& fontManager)
{
    auto convertUpperCase = [](qchar& cp) {
        cp = (qchar)u_toupper((UChar32)cp);
    };

    auto convertLowerCase = [](qchar& cp) {
        cp = (qchar)u_tolower((UChar32)cp);
    };

    const auto& emojiTable = unicode::EmojiTable::instance();

    for (auto i = 0ul; i < text_.size(); ++i) {
        if (format_[i].uppercase) {
            convertUpperCase(text_[i]);
        }

        if (format_[i].lowercase) {
            convertLowerCase(text_[i]);
        }

        if (text_[i] == '\t' && format_[i].tabStops.size() == 0) {
            // symbol is tab, but there's no tabstop record
            // replace tab with large space
            text_[i] = 0x2003 /* Em space*/;
        }

        resolveEmoji(i, fontManager, emojiTable);
    }
}

void FormattedParagraph::resolveEmoji(std::size_t glyphIndex, FontManager& fontManager, const unicode::EmojiTable& emojiTable)
{
    const FaceTable::Item* faceItem = fontManager.facesTable().getFaceItem(format_[glyphIndex].faceId);

    const bool hasGlyph = faceItem && faceItem->face->hasGlyph(text_[glyphIndex]);

    if (!hasGlyph && emojiTable.lookup(text_[glyphIndex])) {
        format_[glyphIndex].faceId = FontManager::DEFAULT_EMOJI_FONT;
        fontManager.setRequiresDefaultEmojiFont();
    }
}

} // namespace priv
} // namespace odtr
