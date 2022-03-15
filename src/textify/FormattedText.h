#pragma once

#include "compat/basic-types.h"
#include "text-format.h"
#include <string>
#include <unordered_set>
#include <vector>


namespace textify {
namespace priv {

class TextParser;

/**
 * The input representation of a formatted text.
 * Has a sequence of Unicode characters, a base format,
 * and a set of format modifiers which change the format for substrings.
 */
class FormattedText
{
    friend class TextParser;

public:
    FormattedText(VerticalAlign verticalAlign,
                  BoundsMode boundsMode,
                  float baseline,
                  HorizontalPositionPolicy horizontalPositioning,
                  BaselinePolicy baselinePolicy);

    FormattedText(FormattedText&&) = default;
    FormattedText& operator=(FormattedText&&) = default;

    FormattedText clone();

    FormattedText deriveEdited(const std::string& newContent);

    FormattedText deriveScaled(float scale) const;

    FormattedText deriveFormatted(const ImmediateFormat& format) const;

    void setBaseFormat(const ImmediateFormat& format);
    const ImmediateFormat& getBaseFormat() const { return baseFormat_; }

    const compat::qchar* getText() const;
    int getLength() const;

    void appendAscii(const char* str, int len = -1);
    void appendUtf8(const char* str, int len = -1);
    void appendUtf16(const uint16_t* str, int len = -1);
    void appendUtf32(const compat::qchar* str, int len = -1);
    void resetText(const std::string& str);
    void commitText();
    void addFormatModifier(const FormatModifier& formatModifier);
    void generateFormat(std::vector<ImmediateFormat>& format) const;
    ImmediateFormat firstFormat() const;

    VerticalAlign verticalAlign() const;
    BoundsMode boundsMode() const;
    float baseline() const;
    HorizontalPositionPolicy horizontalPositionPolicy() const;
    BaselinePolicy baselinePolicy() const;
    inline const std::string& content() const { return content_; }
    inline const std::vector<FormatModifier> & formatModifiers() const { return formatModifiers_; }

    inline void setBaseline(float baseline) { baseline_ = baseline; }
    inline void setContent(const std::string& content) { content_ = content; };
    inline void setBoundsMode(BoundsMode mode) { boundsMode_ = mode; }

    std::string getPreview(int len = 12) const;

    std::unordered_set<std::string> collectUsedFaceNames() const;

    void debug_setHorizontalAlign(HorizontalAlign align);
    void debug_setDefaultFontSize(int fontSize);
    int debug_getDefautlFontSize() const;
    const std::string& debug_getDefautlPostScriptName() const;
    void debug_setDefautlPostScriptName(const std::string& name);
private:
    FormattedText() {} //! used exclusively by TextParser

    FormattedText(const FormattedText& other) = default;

    compat::qchar replace(compat::qchar cp);

    void scale(float scale);

    ImmediateFormat baseFormat_;
    std::vector<compat::qchar> text_;
    std::vector<FormatModifier> formatModifiers_;

    VerticalAlign verticalAlign_;
    BoundsMode boundsMode_;
    float baseline_;
    HorizontalPositionPolicy horizontalPositioning_;
    BaselinePolicy baselinePolicy_;

    /**
     * Original text value encoded as UTF-8 string
     **/
    std::string content_;
};

}
}
