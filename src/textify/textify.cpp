#include "textify.h"
#include "Context.h"

#include "errors.h"
#include "FormattedParagraph.h"
#include "FormattedText.h"
#include "FreetypeHandle.h"
#include "ParagraphShape.h"
#include "TextShape.h"
#include "text-format.h"
#include "TextParser.h"
#include "types.h"

#include "compat/affine-transform.h"
#include "compat/basic-types.h"
#include "compat/Bitmap.hpp"

#include "utils/utils.h"

#include <octopus/text.h>

#include <cmath>
#include <cstdio>
#include <iterator>
#include <optional>

namespace textify {
namespace priv {

namespace {

compat::Rectangle outerRect(const compat::FRectangle& rect) {
    compat::Rectangle result;
    result.l = (int) floorf(rect.l);
    result.t = (int) floorf(rect.t);
    result.w = (int) ceilf(rect.l+rect.w)-result.l;
    result.h = (int) ceilf(rect.t+rect.h)-result.t;
    return result;
}

compat::FRectangle toFRectangle(const compat::Rectangle& r) {
    return {(float)r.l, (float)r.t, (float)r.w, (float)r.h};
}

/**
 * Returns stretched bounds containing bitmap bounds of all the glyphs
 * within typeset journal in @a paragraphResults.
 *
 * @param paragraphResults      paragraphs typeset data
 * @param verticalOffset        vertical offset of contained texts
 *
 * @return bounds object
 */
compat::Rectangle stretchedBounds(const std::vector<ParagraphShape::DrawResult>& paragraphResults, int verticalOffset, int limit)
{
    compat::Rectangle stretchedBounds = {};

    for (const auto& result : paragraphResults) {
        stretchedBounds = stretchedBounds | result.journal.stretchedBounds(verticalOffset, limit);
    }

    return stretchedBounds;
}

compat::FRectangle stretchBounds(const compat::FRectangle& baseBounds, const compat::Rectangle& stretchRect)
{
    compat::FRectangle bounds = baseBounds;
    bounds.t += std::min(stretchRect.t, 0);
    // bounds.l += std::min(stretchRect.l, 0);
    bounds.l = baseBounds.l;
    bounds.h = std::max(baseBounds.h, static_cast<float>(stretchRect.h));
    // bounds.w = std::max(baseBounds.w, stretchRect.w);
    bounds.w = baseBounds.w;
    return bounds;
}

compat::FRectangle scaleRect(const compat::FRectangle& rect, const RenderScale& scale)
{
    if (scale == 1.f)
        return rect;
    float l = scale*rect.l;
    float t = scale*rect.t;
    float r = scale*(rect.l+rect.w);
    float b = scale*(rect.t+rect.h);
    return {l, t, r-l, b-t};
}

/**
 * Split text into formatted paragraphs.
 */
std::vector<FormattedParagraph> splitText(const utils::Log& log, const FormattedText& text, FontManager& fontManager)
{
    // Generate format per characters
    std::vector<ImmediateFormat> textFormat;
    text.generateFormat(textFormat);

    // Split text & format into paragraphs
    std::vector<FormattedParagraph> paragraphs;
    const compat::qchar* textPtr = text.getText();
    const ImmediateFormat* formatPtr = &textFormat[0];

    for (int limit = text.getLength(); limit > 0; ) {
        paragraphs.emplace_back(FormattedParagraph(log));
        int parLen = FormattedParagraph::extractParagraph(paragraphs.back(), textPtr, formatPtr, limit);

        textPtr += parLen;
        formatPtr += parLen;
        limit -= parLen;
    }

    for (auto& p : paragraphs) {
        p.analyzeBidi();

        p.applyFormatModifiers(fontManager);
    }

    return paragraphs;
}

void preserveFixedDimensions(BoundsMode mode, const std::optional<compat::Vector2f>& frameSize, float& w, float& h)
{
    if (!frameSize.has_value() || mode == BoundsMode::AUTO_WIDTH) {
        return;
    }

    const auto& size = frameSize.value();

    if (size.x) {
        w = size.x;
    }

    if (mode == BoundsMode::FIXED && size.y) {
        h = size.y;
    }
}

spacing resolveBaselineOffset(const ParagraphShape::DrawResult& p0, BaselinePolicy baselinePolicy, VerticalAlign verticalAlign)
{
    const bool wantBecauseBaselinePolicy = baselinePolicy == BaselinePolicy::CENTER && verticalAlign != VerticalAlign::CENTER;

    if (wantBecauseBaselinePolicy) {
        // for explicit line height use the value directly
        const spacing lineHeight = p0.firstLineHeight
            ? p0.firstLineHeight
            : p0.firstLineDefaultHeight;

        return 0.5f * (lineHeight - p0.firstAscender + p0.firstDescender);
    }

    return 0.0f;
}

/**
 * Finds a position of the first baseline relative to the top border of a text layer.
 */
spacing resolveBaselinePosition(const ParagraphShape::DrawResult& p0, BaselinePolicy baselinePolicy, VerticalAlign verticalAlign)
{
    spacing baselinePos = 0.0f;
    switch (baselinePolicy) {
        case BaselinePolicy::OFFSET_ASCENDER:
        case BaselinePolicy::SET:
            baselinePos = p0.firstAscender;
            break;
        case BaselinePolicy::OFFSET_BEARING:
            baselinePos = p0.firstLineBearing;
            break;
        case BaselinePolicy::CENTER:
            baselinePos = p0.firstLineBearing + resolveBaselineOffset(p0, baselinePolicy, verticalAlign);
            break;
    }

    return baselinePos;
}

float resolveVerticalOffset(BoundsMode boundsMode, VerticalAlign verticalAlign, const compat::FRectangle& textBounds, float textBottom, spacing baselineOffset)
{
    float verticalOffset = 0.0f;
    if (boundsMode == BoundsMode::FIXED) {
        // vertical alignment is only allowed for fixed bounds
        switch (verticalAlign) {
            case VerticalAlign::CENTER:
                verticalOffset = std::floor((textBounds.h - textBottom) * 0.5f);
                break;

            case VerticalAlign::BOTTOM:
                verticalOffset = std::floor(textBounds.h - textBottom - baselineOffset);
                break;

            default:
                break;
        }
    }
    return verticalOffset;
}

float roundCaretPosition(float pos, bool floorBaseline)
{
    return floorBaseline ? std::floor(pos) : std::round(pos);
}

} // namespace


TextShapeData::TextShapeData(FormattedTextPtr text,
                             FrameSizeOpt frameSize,
                             const compat::Matrix3f& textTransform,
                             ParagraphShapes&& shapes,
                             const compat::FRectangle& boundsNoTransform,
                             const compat::FRectangle& boundsTransformed,
                             float baseline)
    : formattedText(std::move(text)),
      frameSize(frameSize),
      textTransform(textTransform),
      usedFaces(formattedText->collectUsedFaceNames()),
      paragraphShapes(std::move(shapes)),
      textBoundsNoTransform(boundsNoTransform),
      textBoundsTransformed(boundsTransformed),
      baseline(baseline)
{ }


FacesNames listMissingFonts(Context &ctx, const octopus::Text& text)
{
    const TextParser::ParseResult parsedText = TextParser(text).parseText();
    const std::unordered_set<std::string> usedNames = parsedText.text->collectUsedFaceNames();

    FacesNames missing;
    for (auto&& name : usedNames) {
        if (!ctx.fontManager->faceExists(name)) {
            missing.emplace_back(std::move(name));
        }
    }
    return missing;
}

TextShapeResult shapeText(Context &ctx, const octopus::Text& text)
{
    auto parsedText = TextParser(text).parseText();

    FrameSizeOpt frameSize;
    if (text.frame.has_value()) {
        auto [w, h] = text.frame.value().size.value_or(octopus::Dimensions{0.0,0.0});
        frameSize = compat::Vector2f{static_cast<float>(w), static_cast<float>(h)};
    }
    return shapeTextInner(ctx, std::move(parsedText.text), frameSize, parsedText.transformation);
}

TextShapeResult reshapeText(Context &ctx, TextShapeDataPtr&& textShapeData)
{
    return shapeTextInner(ctx, std::move(textShapeData->formattedText), textShapeData->frameSize, textShapeData->textTransform);
}

TextShapeResult shapeTextInner(Context &ctx,
                               FormattedTextPtr formattedText,
                               const FrameSizeOpt &frameSize,
                               const compat::Matrix3f &textTransform)
{
    const utils::Log &log = ctx.getLogger();
    const FormattedText &text = *formattedText.get();

    // Split the text into paragraphs
    const FormattedParagraphs paragraphs = splitText(log, text, ctx.getFontManager());
    if (paragraphs.empty()) {
        return TextShapeError::NO_PARAGRAPHS;
    }

    float maxWidth = text.boundsMode() == BoundsMode::AUTO_WIDTH ? 0.0f : frameSize.value_or(compat::Vector2f{0,0}).x;

    const bool loadGlyphsBearings = text.baselinePolicy() == BaselinePolicy::OFFSET_BEARING;
    // Shape the paragraphs
    ParagraphShapes shapes;
    for (const FormattedParagraph& paragraph : paragraphs) {
        ParagraphShapePtr paragraphShape = std::make_unique<ParagraphShape>(log, ctx.getFontManager().facesTable());
        const ParagraphShape::ShapeResult shapeResult = paragraphShape->shape(paragraph, maxWidth, loadGlyphsBearings);

        if (shapeResult.success) {
            shapes.emplace_back(std::move(paragraphShape));
        }
    }

    if (shapes.empty()) {
        return TextShapeError::SHAPE_ERROR;
    }

    float y = 0.0f;
    VerticalPositioning positioning = VerticalPositioning::TOP_BOUND;

    ParagraphShape::DrawResults paragraphResults = drawParagraphsInner(ctx, shapes, text.baselinePolicy(), text.overflowPolicy(), static_cast<int>(std::floor(maxWidth)), 1.0f, positioning, y);

    // Rerun glyph bitmaps if previous justification was nonsense (zero width for auto-width bounds)
    if (text.boundsMode() == BoundsMode::AUTO_WIDTH) {
        log.debug("Running second pass for text '{}", text.getPreview());

        y = 0;
        positioning = VerticalPositioning::TOP_BOUND;

        maxWidth = 0;
        for (const auto& paragraphResult : paragraphResults) {
            maxWidth = std::max(maxWidth, paragraphResult.maxLineWidth);
        }

        paragraphResults = drawParagraphsInner(ctx, shapes, text.baselinePolicy(), text.overflowPolicy(), static_cast<int>(std::floor(maxWidth)), 1.0f, positioning, y);
    }

    if (paragraphResults.empty()) {
        return TextShapeError::TYPESET_ERROR;
    }

    const ParagraphShape::DrawResult &p0 = paragraphResults[0];

    if (ctx.config.lastLineDescenderOffset) {
        y = std::round(y - paragraphResults.back().lastlineDescender);
    }

    const float firstLineActualHeight = p0.firstAscender + p0.firstDescender;

    float w = 0.0f;
    float h = std::max(std::ceil(y), std::round(ctx.config.preferRealLineHeightOverExplicit ? firstLineActualHeight : p0.firstLineHeight));

    for (const auto& paragraphResult : paragraphResults) {
        w = std::max(w, std::floor(paragraphResult.maxLineWidth));
    }

    const bool isBaselineSet = text.baselinePolicy() == BaselinePolicy::SET;
    const float l = isBaselineSet ? -std::floor(p0.leftFirst) : 0.0f;
    const float t = isBaselineSet ? -std::round(p0.firstAscender) : 0.0f;

    preserveFixedDimensions(text.boundsMode(), frameSize, w, h);

    const compat::FRectangle textBoundsNoTransform { l, t, w, h };
    const compat::FRectangle textBoundsTransform = transform(textBoundsNoTransform, textTransform);

    const float baseline = resolveBaselinePosition(p0, text.baselinePolicy(), text.verticalAlign());

    return std::make_unique<TextShapeData>(
        std::move(formattedText),
        frameSize,
        textTransform,
        std::move(shapes),
        textBoundsNoTransform,
        textBoundsTransform,
        baseline
    );
}

TextDrawResult drawText(Context &ctx,
                        const TextShapeData& shapeData,
                        void* pixels,
                        int width,
                        int height,
                        float scale,
                        bool dry,
                        const compat::Rectangle& viewArea) {
    return drawText(ctx,
                    shapeData.paragraphShapes,
                    shapeData.textTransform,
                    *shapeData.formattedText,
                    shapeData.textBoundsNoTransform,
                    shapeData.baseline,
                    pixels, width, height,
                    scale,
                    dry,
                    viewArea);
}

TextDrawResult drawText(Context &ctx,
                        const ParagraphShapes &paragraphShapes,
                        const compat::Matrix3f &textTransform,
                        const FormattedText &text,
                        const compat::FRectangle &unscaledTextBounds,
                        float baseline,
                        void *pixels,
                        int width,
                        int height,
                        float scale,
                        bool dry,
                        const compat::Rectangle &viewArea) {
    const compat::Matrix3f inverseTransform = inverse(textTransform);
    const compat::FRectangle viewAreaTextSpaceUnscaled = utils::transform(toFRectangle(viewArea), inverseTransform);
    const compat::FRectangle viewAreaTextSpace = scaleRect(viewAreaTextSpace, scale);

    TextDrawResult drawResult = drawTextInner(
            ctx,
            dry,
            text,
            unscaledTextBounds,
            baseline,
            scale, viewAreaTextSpace,
            paragraphShapes,
            static_cast<Pixel32*>(pixels), width, height);

    if (drawResult) {
        TextDrawOutput value = drawResult.moveValue();
        value.transform = textTransform;
        value.transform[2][0] *= scale;
        value.transform[2][1] *= scale;
        return value;
    } else {
        return drawResult.error();
    }
}

TextDrawResult drawTextInner(Context &ctx,
                             bool dry,
                             const FormattedText& text,
                             const compat::FRectangle& unscaledTextBounds,
                             float baseline,
                             RenderScale scale,
                             const compat::FRectangle& viewArea,
                             const ParagraphShapes& paragraphShapes,
                             compat::Pixel32* pixels,
                             int width,
                             int height) {
    if (text.getLength() == 0) {
        return TextDrawOutput{{}};
    }

    compat::BitmapRGBA output(compat::BitmapRGBA::WRAP_NO_OWN, pixels, width, height);

    const compat::FRectangle textBounds = scaleRect(unscaledTextBounds, scale);
    if (!textBounds) {
        return TextDrawError::INVALID_SCALE;
    }

    VerticalPositioning positioning = VerticalPositioning::BASELINE;
    float caretVerticalPos = roundCaretPosition(baseline * scale, ctx.config.floorBaseline);

    const OverflowPolicy overflowPolicy = text.overflowPolicy();
    const BaselinePolicy baselinePolicy = text.baselinePolicy();
    const VerticalAlign verticalAlign = text.verticalAlign();
    const BoundsMode boundsMode = text.boundsMode();

    const ParagraphShape::DrawResults paragraphResults = drawParagraphsInner(ctx, paragraphShapes, baselinePolicy, overflowPolicy, textBounds.w, scale, positioning, caretVerticalPos);
    if (paragraphResults.empty()) {
        return TextDrawError::PARAGRAPHS_TYPESETING_ERROR;
    }

    // account for descenders of last paragraph's last line
    const spacing textBottom = caretVerticalPos - paragraphResults.back().lastlineDescender;

    const spacing baselineOffset = resolveBaselineOffset(paragraphResults.front(), baselinePolicy, verticalAlign);
    const float verticalOffset = resolveVerticalOffset(boundsMode, verticalAlign, textBounds, textBottom, baselineOffset * scale);

    const bool unlimitedVerticalStretch = overflowPolicy == OverflowPolicy::EXTEND_ALL || boundsMode == BoundsMode::AUTO_HEIGHT || verticalAlign != VerticalAlign::TOP;

    const float verticalStretchLimit = unlimitedVerticalStretch ? 0.0f : textBounds.h;
    compat::Rectangle stretchedGlyphsBounds;
    if (boundsMode != BoundsMode::FIXED || overflowPolicy != OverflowPolicy::NO_OVERFLOW) {
        stretchedGlyphsBounds = stretchedBounds(paragraphResults, int(verticalOffset), verticalStretchLimit);
    }
    compat::FRectangle stretchedTextBounds = stretchBounds(textBounds, stretchedGlyphsBounds);

    compat::Rectangle bitmapBounds = outerRect(stretchedTextBounds);

    if (!bitmapBounds) {
        return TextDrawError::DRAW_BOUNDS_ERROR;
    }

    if (ctx.config.enableViewAreaCutout) {
        bitmapBounds = outerRect(viewArea & stretchedTextBounds);
    }

    if (dry) {
        return TextDrawOutput{bitmapBounds};
    }

    compat::Vector2i offset = {0, 0};
    compat::Rectangle viewAreaBounds = compat::INFINITE_BOUNDS;

    if (ctx.config.enableViewAreaCutout) {
        const compat::Rectangle offsetBounds = outerRect(stretchedTextBounds) - bitmapBounds;

        viewAreaBounds = outerRect(viewArea);
        offset = compat::Vector2i{offsetBounds.l, offsetBounds.t};
    }

    // vertical align offset
    offset.y += int(verticalOffset - stretchedGlyphsBounds.t);

    for (const auto& paragraphResult : paragraphResults) {
        const TypesetJournal::DrawResult journalDrawResult = paragraphResult.journal.draw(output, bitmapBounds, stretchedTextBounds.h, viewAreaBounds, offset);

        if (journalDrawResult) {
            // totalGlyphsDrawn += (decltype(totalGlyphsDrawn)) journalDrawResult.value().numGlyphsRendered;
            // totalGlyphBlitTime += journalDrawResult.value().glyphsRenderTime.timeSpan();
        }
    }

    return TextDrawOutput{bitmapBounds};
}

ParagraphShape::DrawResults drawParagraphsInner(Context &ctx,
                                                const ParagraphShapes &shapes,
                                                BaselinePolicy baselinePolicy,
                                                OverflowPolicy overflowPolicy,
                                                int textWidth,
                                                RenderScale scale,
                                                VerticalPositioning &positioning,
                                                float &caretVerticalPos) {
    ParagraphShape::DrawResults drawResults;

    const bool hasBaselineTransform = baselinePolicy == BaselinePolicy::SET;

    for (const ParagraphShapePtr& paragraphShape : shapes) {
        const bool isLast = (paragraphShape == shapes.back());
        ParagraphShape::DrawResult drawResult = paragraphShape->draw(ctx, 0, textWidth, caretVerticalPos, positioning, scale, isLast, hasBaselineTransform, false);

        const LastLinePolicy lastLinePolicy =
            (overflowPolicy == OverflowPolicy::CLIP_LINE && drawResult.journal.size() > 1)
            ? LastLinePolicy::CUT
            : LastLinePolicy::FORCE;
        drawResult.journal.setLastLinePolicy(lastLinePolicy);

        drawResults.emplace_back(std::move(drawResult));
    }

    return drawResults;
}

} // namespace priv
} // namespace textify
