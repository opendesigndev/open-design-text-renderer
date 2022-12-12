#pragma once

#include "base.h"
#include "text-format.h"


namespace textify {

namespace unicode {
class EmojiTable;
}

namespace utils {
class Log;
}

class FontManager;

namespace priv {

/**
 * Internal representation of a single paragraph, with format properties evaluated for each character.
 */
class FormattedParagraph
{
    friend class ParagraphShape;

public:
    static int extractParagraph(FormattedParagraph& output, const compat::qchar* string, const ImmediateFormat* format, int limit);

    explicit FormattedParagraph(const utils::Log& log);
    const compat::qchar* getText() const;
    const ImmediateFormat* getFormat() const;
    int getLength() const;
    void append(compat::qchar character, const ImmediateFormat& format);
    void append(const compat::qchar* characters, const ImmediateFormat* format, int len);
    bool analyzeBidi();
    void applyFormatModifiers(FontManager& fontManager);

private:
    /**
     * If a glyph at given @a glyphIndex encodes an emoji symbol and it's not contained in the original face,
     * the face used is set to default emoji instead.
     */
    void resolveEmoji(std::size_t glyphIndex, FontManager& fontManager, const unicode::EmojiTable& emojiTable);

    const utils::Log& log_;

    std::vector<compat::qchar> text_;
    std::vector<ImmediateFormat> format_;
    std::vector<VisualRun> visualRuns_;
    TextDirection baseDirection_;
};
using FormattedParagraphs = std::vector<FormattedParagraph>;

}
} // namespace textify
