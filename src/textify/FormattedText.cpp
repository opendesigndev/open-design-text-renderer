#include "FormattedText.h"

#include <algorithm>
#include <cmath>
#include <locale>

#include "textify/text-format.h"
#include "fonts/FontManager.h"
#include "unicode/EmojiTable.h"


namespace textify {
namespace priv {

using namespace compat;

FormattedText::FormattedText(VerticalAlign verticalAlign,
                             BoundsMode boundsMode,
                             float baseline,
                             HorizontalPositionPolicy horizontalPositioning,
                             BaselinePolicy baselinePolicy,
                             OverflowPolicy overflowPolicy)
    : params_(FormattingParams { verticalAlign, boundsMode, baseline, horizontalPositioning, baselinePolicy, overflowPolicy })
{
}

FormattedText FormattedText::clone()
{
    return *this;
}

FormattedText FormattedText::deriveEdited(const std::string& newContent)
{
    FormattedText edited(*this);

    edited.setBaseFormat(firstFormat());
    edited.formatModifiers_.clear();
    edited.resetText(newContent);

    return edited;
}

FormattedText FormattedText::deriveScaled(float scale) const
{
    FormattedText scaled(*this);
    scaled.scale(scale);

    return scaled;
}

FormattedText FormattedText::deriveFormatted(const ImmediateFormat& format) const
{
    FormattedText formatted(*this);

    formatted.setBaseFormat(format);
    formatted.formatModifiers_.clear();

    return formatted;
}

const qchar* FormattedText::getText() const
{
    return text_.data();
}

int FormattedText::getLength() const
{
    return static_cast<int>(text_.size());
}

void FormattedText::setBaseFormat(const ImmediateFormat& format)
{
    baseFormat_ = format;
}

void FormattedText::appendAscii(const char* str, int len)
{
    if (len >= 0) {
        while (len--) {
            text_.push_back((qchar)(unsigned char)*str++);
        }
    } else {
        while (*str) {
            text_.push_back((qchar)(unsigned char)*str++);
        }
    }
}

void FormattedText::appendUtf8(const char* str, int len)
{
    qchar character = 0;
    int charLen = 0;
    for (; len > 0 || (len < 0 && *str); ++str, --len) {
        int input = (int)(unsigned char)*str;
        if ((input & 0x80) == 0x00) {
            // Code point fits into 7 bits
            character = (qchar)input;
            charLen = 0;
        } else if ((input & 0xc0) == 0xc0) {
            // Starting byte of multibyte codepoint, get number of following bytes
            for (charLen = 1; (input << charLen) & 0x40; ++charLen)
                ;
            character = (input & 0x3f >> charLen) << (6 * charLen);
        } else if ((input & 0xc0) == 0x80) {
            // Following byte of multibyte codepoint
            if (charLen > 0) {
                --charLen;
                character |= (input & 0x3f) << (6 * charLen);
            } else
                continue; // invalid UTF-8
        }
        if (charLen == 0) {
            text_.push_back(character);
        }
    }
}

void FormattedText::appendUtf16(const uint16_t* str, int len)
{
    int highSurrogate = 0;
    for (; len > 0 || (len < 0 && *str); ++str, --len) {
        int input = (int)(unsigned short)*str;
        if ((input & 0xfc00) == 0xd800)
            highSurrogate = input;
        else if ((input & 0xfc00) == 0xdc00) {
            if (highSurrogate != 0) {
                text_.push_back((qchar)((highSurrogate & 0x03ff) << 10 | (input & 0x03ff)) + 0x00010000u);
            }
        } else {
            text_.push_back((qchar)input);
        }
        highSurrogate = 0;
    }
}

void FormattedText::appendUtf32(const qchar* str, int len)
{
    if (len >= 0) {
        while (len--) {
            text_.push_back(*str++);
        }
    } else {
        while (*str) {
            text_.push_back(*str++);
        }
    }
}

void FormattedText::resetText(const std::string& str)
{
    text_.clear();
    appendUtf8(str.c_str());
    commitText();

    setContent(str);
}

void FormattedText::commitText()
{
    for (auto& cp : text_) {
        cp = replace(cp);
    }
}

void FormattedText::addFormatModifier(const FormatModifier& formatModifier)
{
    formatModifiers_.push_back(formatModifier);
}

void FormattedText::generateFormat(std::vector<ImmediateFormat>& format) const
{
    // update modifier values to account for various multi byte encoding
    // octopus is utf-8 while the modifier ranges are utf-16
    std::vector<int> cumulativeLengths;
    cumulativeLengths.reserve(text_.size() + 1);
    cumulativeLengths.push_back(0);
    for (int i = 0; i < text_.size(); ++i) {
        cumulativeLengths.push_back(cumulativeLengths[i] + 1);
    }

    const auto textLen = static_cast<unsigned int>(text_.size());
    format.assign(textLen, baseFormat_);
    for (const auto& modifier : formatModifiers_) {
        auto pos = std::distance(cumulativeLengths.begin(),
                                 std::find(cumulativeLengths.begin(), cumulativeLengths.end(), modifier.range.start));
        auto end = std::distance(cumulativeLengths.begin(),
                                 std::find(cumulativeLengths.begin(), cumulativeLengths.end(), modifier.range.end));

        for (; pos < end && pos < textLen; ++pos) {
            if (modifier.types & FormatModifier::FACE) {
                format[pos].faceId = modifier.face;
            }
            if (modifier.types & FormatModifier::SIZE) {
                format[pos].size = modifier.size;
            }
            if (modifier.types & FormatModifier::LINE_HEIGHT) {
                format[pos].lineHeight = modifier.lineHeight;
                format[pos].minLineHeight = modifier.minLineHeight;
                format[pos].maxLineHeight = modifier.maxLineHeight;
            }
            if (modifier.types & FormatModifier::LETTER_SPACING) {
                format[pos].letterSpacing = modifier.letterSpacing;
            }
            if (modifier.types & FormatModifier::PARAGRAPH_SPACING) {
                format[pos].paragraphSpacing = modifier.paragraphSpacing;
            }
            if (modifier.types & FormatModifier::COLOR) {
                format[pos].color = modifier.color;
            }
            if (modifier.types & FormatModifier::DECORATION) {
                format[pos].decoration = modifier.decoration;
            }
            if (modifier.types & FormatModifier::ALIGN) {
                format[pos].align = modifier.align;
            }
            if (modifier.types & FormatModifier::KERNING) {
                format[pos].kerning = modifier.kerning;
            }
            if (modifier.types & FormatModifier::LIGATURES) {
                format[pos].ligatures = modifier.ligatures;
            }
            if (modifier.types & FormatModifier::UPPERCASE) {
                format[pos].uppercase = modifier.uppercase;
                format[pos].lowercase = !modifier.uppercase;
            }
            if (modifier.types & FormatModifier::LOWERCASE) {
                format[pos].lowercase = modifier.lowercase;
                format[pos].uppercase = !modifier.lowercase;
            }
            if (modifier.types & FormatModifier::TYPE_FEATURE) {
                format[pos].features = modifier.features;
            }
            if (modifier.types & FormatModifier::TAB_STOPS) {
                format[pos].tabStops = modifier.tabStops;
            }
        }
    }
}

ImmediateFormat FormattedText::firstFormat() const
{
    std::vector<ImmediateFormat> textFormat;
    generateFormat(textFormat);


    return textFormat.empty() ? baseFormat_ : textFormat[0];
    return textFormat[0];
}

const FormattedText::FormattingParams &FormattedText::formattingParams() const {
    return params_;
}

VerticalAlign FormattedText::verticalAlign() const
{
    return params_.verticalAlign;
}

BoundsMode FormattedText::boundsMode() const
{
    return params_.boundsMode;
}

float FormattedText::baseline() const
{
    return params_.baseline;
}

HorizontalPositionPolicy FormattedText::horizontalPositionPolicy() const
{
    return params_.horizontalPositioning;
}

BaselinePolicy FormattedText::baselinePolicy() const
{
    return params_.baselinePolicy;
}

OverflowPolicy FormattedText::overflowPolicy() const
{
    return params_.overflowPolicy;
}

std::string FormattedText::getPreview(int len) const
{
    if (len < 0)
        len = 12;

    std::string preview(len + 1, '\0');
    auto n = sprintf(&preview[0], "%.*s", len, content_.c_str());
    preview.resize(n);

    for (auto& c : preview) {
        if (c == '\n')
            c = ' ';
    }

    if (content_.length() > len)
        preview += "...";

    return preview;
}

std::unordered_set<std::string> FormattedText::collectUsedFaceNames() const
{
    std::unordered_set<std::string> faces;
    if (!baseFormat_.faceId.empty()) {
        faces.insert(baseFormat_.faceId);
    }

    for (const auto& mod : formatModifiers_) {
        if (mod.FACE) {
            faces.insert(mod.face);
        }
    }

    const unicode::EmojiTable &emojiTable = unicode::EmojiTable::instance();
    for (compat::qchar ch : text_) {
        if (emojiTable.lookup(ch)) {
            faces.insert(FontManager::DEFAULT_EMOJI_FONT);
            break;
        }
    }

    return faces;
}

void FormattedText::debug_setHorizontalAlign(HorizontalAlign align)
{
    baseFormat_.align = align;

    for (auto& fmt : formatModifiers_) {
        fmt.align  = align;
    }
}

void FormattedText::debug_setDefaultFontSize(int fontSize)
{
    baseFormat_.size = fontSize;
}

int FormattedText::debug_getDefautlFontSize() const
{
    return baseFormat_.size;
}

const std::string& FormattedText::debug_getDefautlPostScriptName() const
{
    return baseFormat_.faceId;

}

void FormattedText::debug_setDefautlPostScriptName(const std::string& name)
{
    baseFormat_.faceId = name;

    for (auto& fmt : formatModifiers_) {
        fmt.face = name;
    }
}

qchar FormattedText::replace(qchar cp)
{
    if (cp == (qchar)0x0003) {
        return (qchar)' ';
    }
    // additional replacement

    return cp;
}

void FormattedText::scale(float scale)
{
    baseFormat_.size = font_size(scale*baseFormat_.size);

    baseFormat_.lineHeight *= scale;
    baseFormat_.minLineHeight *= scale;
    baseFormat_.maxLineHeight *= scale;

    baseFormat_.letterSpacing *= scale;
    baseFormat_.paragraphSpacing *= scale;
    baseFormat_.paragraphIndent *= scale;

    for (auto& fmt : formatModifiers_) {
        fmt.size = font_size(scale*fmt.size);

        fmt.lineHeight *= scale;
        fmt.minLineHeight *= scale;
        fmt.maxLineHeight *= scale;

        fmt.letterSpacing *= scale;
        fmt.paragraphSpacing *= scale;
        fmt.paragraphIndent *= scale;
    }
}

} // namespace priv
} // namespace textify
