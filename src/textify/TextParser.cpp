#include "TextParser.h"

#include "FormattedText.h"
#include "compat/affine-transform.h"
#include "compat/basic-types.h"
#include "compat/pixel-conversion.h"
#include "text-format.h"
// #include "parser_common.h"

#include <cstdint>
#include <memory>
#include <octopus/text.h>

#include <cstdio>
#include <cassert>

namespace {
TypeFeatures convertFeatures(const std::vector<octopus::OpenTypeFeature>& features)
{
    TypeFeatures outFeatures;

    for(const octopus::OpenTypeFeature& feature : features) {
        outFeatures.emplace_back(TypeFeature{feature.tag, feature.value});
    }

    return outFeatures;
}
}

namespace textify {
namespace priv {

TextParser::TextParser(const octopus::Text& text)
    : text(text)
{}

TextParser::ParseResult TextParser::parseText() const
{
    ParseResult res;

    auto verticalAlign         = parseVerticalAlign();
    auto boundsMode            = parseBoundsMode();
    auto horizontalPositioning = HorizontalPositionPolicy::ALIGN_TO_FRAME;
    auto baselinePolicy        = parseBaselinePolicy();
    auto overflowPolicy        = parseOverflowPolicy();

    res.text = std::make_unique<FormattedText>(verticalAlign, boundsMode, 0.0f, horizontalPositioning, baselinePolicy, overflowPolicy);
    res.text->setBaseFormat(parseBaseFormat(text.defaultStyle));

    parseStyles(&res);

    res.text->appendUtf8(text.value.c_str(), static_cast<int>(text.value.size()));
    res.text->commitText();
    res.text->setContent(text.value);

    res.transformation = parseTransformation();

    return res;
}

VerticalAlign TextParser::parseVerticalAlign() const
{
    switch (text.verticalAlign) {
        case octopus::Text::VerticalAlign::TOP:
            return VerticalAlign::TOP;

        case octopus::Text::VerticalAlign::CENTER:
            return VerticalAlign::CENTER;

        case octopus::Text::VerticalAlign::BOTTOM:
            return VerticalAlign::BOTTOM;
    }
    return { };
}

HorizontalAlign TextParser::parseHorizontalAlign() const
{
    switch (text.horizontalAlign) {
        case octopus::Text::HorizontalAlign::LEFT:
            return HorizontalAlign::LEFT;

        case octopus::Text::HorizontalAlign::RIGHT:
            return HorizontalAlign::RIGHT;

        case octopus::Text::HorizontalAlign::CENTER:
            return HorizontalAlign::CENTER;

        case octopus::Text::HorizontalAlign::JUSTIFY:
            return HorizontalAlign::JUSTIFY;
    }
    return { };
}

BoundsMode TextParser::parseBoundsMode() const
{
    switch(text.frame.value_or(octopus::TextFrame{}).mode) {
        case octopus::TextFrame::Mode::AUTO_WIDTH:
            return BoundsMode::AUTO_WIDTH;

        case octopus::TextFrame::Mode::AUTO_HEIGHT:
            return BoundsMode::AUTO_HEIGHT;

        case octopus::TextFrame::Mode::FIXED:
            return BoundsMode::FIXED;
    }
    return { };
}

BaselinePolicy TextParser::parseBaselinePolicy() const
{
    switch(text.baselinePolicy) {
        case octopus::Text::BaselinePolicy::SET:
            return BaselinePolicy::SET;

        case octopus::Text::BaselinePolicy::CENTER:
            return BaselinePolicy::CENTER;

        case octopus::Text::BaselinePolicy::OFFSET_ASCENDER:
            return BaselinePolicy::OFFSET_ASCENDER;

        case octopus::Text::BaselinePolicy::OFFSET_BEARING:
            return BaselinePolicy::OFFSET_BEARING;
    }
    return { };
}

OverflowPolicy TextParser::parseOverflowPolicy() const
{
    switch(text.overflowPolicy) {
        case octopus::Text::OverflowPolicy::NO_OVERFLOW:
            return OverflowPolicy::NO_OVERFLOW;

        case octopus::Text::OverflowPolicy::CLIP_LINE:
            return OverflowPolicy::CLIP_LINE;

        case octopus::Text::OverflowPolicy::EXTEND_LINE:
            return OverflowPolicy::EXTEND_LINE;

        case octopus::Text::OverflowPolicy::EXTEND_ALL:
            return OverflowPolicy::EXTEND_ALL;
    }
    return { };
}

constexpr float deg2rad(float deg) {
    return 3.1415926 * deg / 180.0f;
}

compat::Matrix3f TextParser::parseTransformation() const
{
    auto t = text.transform;
    auto transformation = compat::Matrix3f::identity;

    transformation[0][0] = t[0];
    transformation[0][1] = t[1];
    transformation[1][0] = t[2];
    transformation[1][1] = t[3];
    transformation[2][0] = t[4];
    transformation[2][1] = t[5];

    return transformation;
}

ImmediateFormat TextParser::parseBaseFormat(const octopus::TextStyle& style) const
{
    // const auto& font = style.font;
    ImmediateFormat baseFormat = {};

    baseFormat.align            = parseHorizontalAlign();
    baseFormat.faceId           = style.font.has_value() ? style.font.value().postScriptName : "";
    baseFormat.size             = style.fontSize.value_or(0.0);
    baseFormat.lineHeight       = style.lineHeight.value_or(0.0);
    baseFormat.minLineHeight    = 0.0f;
    baseFormat.maxLineHeight    = 0.0f;
    baseFormat.letterSpacing    = style.letterSpacing.value_or(0.0);
    baseFormat.paragraphSpacing = 0.0f;
    baseFormat.paragraphIndent  = 0.0f;
    baseFormat.color            = parseColor(style);
    baseFormat.decoration       = parseUnderline(style);
    baseFormat.kerning          = style.kerning.value_or(true);
    baseFormat.uppercase        = style.letterCase.value_or(octopus::TextStyle::LetterCase::NONE) == octopus::TextStyle::LetterCase::UPPERCASE;
    baseFormat.lowercase        = style.letterCase.value_or(octopus::TextStyle::LetterCase::NONE) == octopus::TextStyle::LetterCase::LOWERCASE;
    baseFormat.ligatures        = parseLigatures(style);
    baseFormat.features         = style.features.has_value() ? convertFeatures(style.features.value()) : TypeFeatures{};
    // baseFormat.tabStops         = style.tabStops; // TODO: implement tab stops

    return baseFormat;
}

Decoration TextParser::parseUnderline(const octopus::TextStyle& style) const
{
    switch(style.underline.value_or(octopus::TextStyle::Underline::NONE)) {
        case octopus::TextStyle::Underline::SINGLE:
            return Decoration::UNDERLINE;

        case octopus::TextStyle::Underline::DOUBLE:
            return Decoration::DOUBLE_UNDERLINE;

        default:
            return Decoration::NONE;
    }
}

GlyphFormat::Ligatures TextParser::parseLigatures(const octopus::TextStyle& style) const
{
    switch(style.ligatures.value_or(octopus::TextStyle::Ligatures::STANDARD)) {
        case octopus::TextStyle::Ligatures::NONE:
            return GlyphFormat::Ligatures::NONE;

        case octopus::TextStyle::Ligatures::ALL:
            return GlyphFormat::Ligatures::ALL;

        default:
            return GlyphFormat::Ligatures::STANDARD;
    }
}

compat::Pixel32 TextParser::parseColor(const octopus::TextStyle& style) const
{
    compat::Pixel32 p = 0;
    if (style.fills.has_value() && !style.fills->empty() && style.fills->front().type == octopus::Fill::Type::COLOR && style.fills->front().color.has_value()) {
        const octopus::Color &c = *style.fills->front().color;
        p = compat::colorToPixel(compat::Color::makeRGBA(c.r, c.g, c.b, c.a));
    }
    return p;
}

void TextParser::parseStyles(ParseResult* result) const
{
    if (!text.styles.has_value()) {
        return;
    }

    for (const auto& styleRange : text.styles.value()) {
        FormatModifier modifier = parseStyle(styleRange.style);
        if (modifier.types) {
            for (const auto& [from, to] : styleRange.ranges) {
                modifier.range.start = from;
                modifier.range.end = to;
                result->text->addFormatModifier(modifier);
            }
        }
    }
}

FormatModifier TextParser::parseStyle(const octopus::TextStyle& style) const
{
    FormatModifier modifier = {};

    if (style.font.has_value()) {
        modifier.types |= FormatModifier::FACE;
        modifier.face = style.font.value().postScriptName;
    }
    if (style.fontSize.has_value()) {
        modifier.types |= FormatModifier::SIZE;
        modifier.size = static_cast<font_size>(style.fontSize.value());
    }
    if (style.lineHeight.has_value()) {
        modifier.types |= FormatModifier::LINE_HEIGHT;
        modifier.lineHeight = style.lineHeight.value();
    }
    if (style.letterSpacing.has_value()) {
        modifier.types |= FormatModifier::LETTER_SPACING;
        modifier.letterSpacing = style.letterSpacing.value();
    }
    /*
    if (style.font.paragraphSpacing.has_value()) {
        modifier.types |= FormatModifier::PARAGRAPH_SPACING;
        modifier.paragraphSpacing = style.font.paragraphSpacing.value();
    }
    if (style.font.paragraphIndent.has_value()) {
        modifier.types |= FormatModifier::PARAGRAPH_INDENT;
        modifier.paragraphIndent = style.font.paragraphIndent.value();
    }
    */
    if (style.fills.has_value()) {
        modifier.types |= FormatModifier::COLOR;
        modifier.color = parseColor(style);
    }
    if (style.underline.has_value()) {
        modifier.types |= FormatModifier::DECORATION;
        modifier.decoration = parseUnderline(style);
    }
    if (style.linethrough.has_value()) {
        modifier.types |= FormatModifier::DECORATION;
        modifier.decoration = style.linethrough.value() ? Decoration::STRIKE_THROUGH : modifier.decoration;
    }
    if (style.kerning.has_value()) {
        modifier.types |= FormatModifier::KERNING;
        modifier.kerning = style.kerning.value();
    }
    if (style.ligatures.has_value()) {
        modifier.types |= FormatModifier::LIGATURES;
        modifier.ligatures = parseLigatures(style);
    }
    if (style.letterCase.has_value()) {
        if (style.letterCase.value() == octopus::TextStyle::LetterCase::UPPERCASE) {
            modifier.uppercase = true;
            modifier.types |= FormatModifier::UPPERCASE;
        } else if (style.letterCase.value() == octopus::TextStyle::LetterCase::LOWERCASE) {
            modifier.lowercase = true;
            modifier.types |= FormatModifier::LOWERCASE;
        }
    }
    if (style.features.has_value()) {
        modifier.types |= FormatModifier::TYPE_FEATURE;
        modifier.features = convertFeatures(style.features.value());
    }
    /*
    // TODO: tabstops
    if (!style.tabStops.empty()) {
        modifier.types |= FormatModifier::TAB_STOPS;
        modifier.tabStops = style.tabStops;
    }
    */

    return modifier;
}

} // namespace priv
} // namespace textify
