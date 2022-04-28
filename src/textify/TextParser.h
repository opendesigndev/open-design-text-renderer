#pragma once

#include "FormattedText.h"
#include "text-format.h"
#include <memory>

namespace octopus {
    struct Text;
    struct TextStyle;
}

namespace textify {
namespace priv {

class TextParser
{
public:
    struct ParseResult
    {
        std::unique_ptr<FormattedText> text;
        compat::Matrix3f transformation = compat::Matrix3f::identity;
    };

    TextParser() = delete;

    /**
     * Specify the text this parser should be working with.
     *
     * @param text      TextParser holds a const reference to the text, do not
     *                  delete while using.
     */
    TextParser(const octopus::Text& text);

    /**
     * Parse text specified in the ctor. This method calls all relevant private
     * methods, do not call them specifically.
     *
     * @return All information needed to create a text layer except for bounds,
     *         use #parseBounds for that.
     */
    ParseResult parseText() const;


private:
    ImmediateFormat parseBaseFormat(const octopus::TextStyle& style) const;
    VerticalAlign parseVerticalAlign() const;
    HorizontalAlign parseHorizontalAlign() const;
    BoundsMode parseBoundsMode() const;
    BaselinePolicy parseBaselinePolicy() const;
    compat::Matrix3f parseTransformation() const;
    Decoration parseDecoration(const octopus::TextStyle& style) const;
    GlyphFormat::Ligatures parseLigatures(const octopus::TextStyle& style) const;
    compat::Pixel32 parseColor(const octopus::TextStyle& style) const;
    void parseStyles(ParseResult* result) const;
    FormatModifier parseStyle(const octopus::TextStyle& style) const;

    const octopus::Text& text;
};

}
}
