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

#include "utils/fmt.h"
#include "utils/utils.h"

#include <octopus/text.h>

#include <cmath>
#include <cstdio>
#include <iterator>
#include <optional>

namespace textify {
namespace priv {

TextShapeData::TextShapeData(FormattedTextPtr text,
                             MayBeFrameSize frameSize,
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
compat::Rectangle
stretchedBounds(const std::vector<ParagraphShape::DrawResult>& paragraphResults, int verticalOffset, int limit)
{
    compat::Rectangle stretchedBounds = {};

    for (const auto& result : paragraphResults) {
        stretchedBounds = stretchedBounds | result.journal.stretchedBounds(verticalOffset, limit);
    }

    return stretchedBounds;
}

compat::FRectangle stretchBounds(const compat::FRectangle& baseBounds, const compat::Rectangle& stretchRect)
{
    auto bounds = baseBounds;
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

spacing resolveBaselineOffset(const FormattedText& text, const ParagraphShape::DrawResult& p0)
{
    spacing offset = 0.0f;

    auto wantBecauseBaselinePolicy = text.baselinePolicy() == BaselinePolicy::CENTER && text.verticalAlign() != VerticalAlign::CENTER;

    if (wantBecauseBaselinePolicy) {
        auto lineHeight = p0.firstLineDefaultHeight;

        if (p0.firstLineHeight) {
            // for explicit line height use the value directly
            lineHeight = p0.firstLineHeight;
        }

        offset = 0.5f * (lineHeight - p0.firstAscender + p0.firstDescender);
    }

    return offset;
}

/**
 * Finds a position of the first baseline relative to the top border of a text layer.
 */
spacing resolveBaselinePosition(const FormattedText& text, const ParagraphShape::DrawResult& p0)
{
    spacing baselinePos = 0.0f;
    switch (text.baselinePolicy()) {
        case BaselinePolicy::OFFSET_ASCENDER:
        case BaselinePolicy::SET:
            baselinePos = p0.firstAscender;
            break;
        case BaselinePolicy::OFFSET_BEARING:
            baselinePos = p0.firstLineBearing;
            break;
        case BaselinePolicy::CENTER:
            baselinePos = p0.firstLineBearing + resolveBaselineOffset(text, p0);
            break;
    }

    return baselinePos;
}

float resolveVerticalAlignment(const FormattedText& text, const compat::FRectangle& textBounds, float textBottom, spacing baselineOffset)
{
    auto verticalOffset = 0.0f;
    if (text.boundsMode() == BoundsMode::FIXED) {
        // vertical alignment is only allowed for fixed bounds
        switch (text.verticalAlign()) {
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
    if (floorBaseline) {
        return std::floor(pos);
    }

    return std::round(pos);
}

}

FacesNames listMissingFonts(Context* ctx, const octopus::Text& text)
{
    const auto& fontManager = ctx->getFontManager();

    auto parsedText = TextParser(text).parseText();
    auto usedNames = parsedText.text->collectUsedFaceNames();

    FacesNames missing;
    for (auto&& name : usedNames) {
        if (!ctx->fontManager->faceExists(name)) {
            missing.emplace_back(std::move(name));
        }
    }
    return missing;
}

TextShapeResult shapeText(Context* ctx, const octopus::Text& text)
{
    auto parsedText = TextParser(text).parseText();

    MayBeFrameSize frameSize;
    if (text.frame.has_value()) {
        auto [w, h] = text.frame.value().size.value_or(octopus::Dimensions{0.0,0.0});
        frameSize = compat::Vector2f{static_cast<float>(w), static_cast<float>(h)};
    }
    return shapeTextInner(*ctx, std::move(parsedText.text), frameSize, parsedText.transformation);
}

TextShapeResult reshapeText(Context* ctx, TextShapeDataPtr&& textShapeData)
{
    return shapeTextInner(*ctx, std::move(textShapeData->formattedText), textShapeData->frameSize, textShapeData->textTransform);
}

TextShapeResult shapeTextInner(Context& ctx,
                               std::unique_ptr<FormattedText> formattedText,
                               const std::optional<compat::Vector2f>& frameSize,
                               const compat::Matrix3f& textTransform)
{
    const auto& log = ctx.getLogger();
    const auto& text = *formattedText.get();

    // Split the text into paragraphs
    auto paragraphs = splitText(log, text, ctx.getFontManager());

    if (paragraphs.empty()) {
        return TextShapeError::NO_PARAGRAPHS;
    }

    auto maxWidth = text.boundsMode() == BoundsMode::AUTO_WIDTH ? 0.0f : frameSize.value_or(compat::Vector2f{0,0}).x;

    // Shape the paragraphs
    std::vector<std::unique_ptr<ParagraphShape>> shapes;
    for (const auto& paragraph : paragraphs) {
        auto loadBearing = text.baselinePolicy() == BaselinePolicy::OFFSET_BEARING;
        auto paragraphShape = std::make_unique<ParagraphShape>(log, ctx.getFontManager(), loadBearing);
        auto shapeResult = paragraphShape->shape(paragraph, maxWidth);

        if (shapeResult.success) {
            shapes.emplace_back(std::move(paragraphShape));
        }
    }

    if (shapes.empty()) {
        return TextShapeError::SHAPE_ERROR;
    }

    std::vector<ParagraphShape::DrawResult> paragraphResults;

    auto y = 0.0f;
    auto positioning = VerticalPositioning::TOP_BOUND;
    auto baselineSet = text.baselinePolicy() == BaselinePolicy::SET;

    // TODO: move to a standalone function
    auto drawShapes = [&]() {
        for (const auto& paragraphShape : shapes) {
            auto last = paragraphShape == shapes.back();
            auto drawResult =
                paragraphShape->draw(ctx, 0, static_cast<int>(std::floor(maxWidth)), y, positioning, 1.0f, last, baselineSet, false);

            const auto lastLinePolicy =
                (ctx.config.cutLastLine && (paragraphs.size() > 1 || drawResult.journal.size() > 1))
                    ? LastLinePolicy::CUT
                    : LastLinePolicy::FORCE;
            drawResult.journal.setLastLinePolicy(lastLinePolicy);

            paragraphResults.emplace_back(std::move(drawResult));
        }
    };

    drawShapes();

    // Rerun glyph bitmaps if previous justification was nonsense (zero width for auto-width bounds)
    if (text.boundsMode() == BoundsMode::AUTO_WIDTH) {
        log.debug("Running second pass for text '{}", text.getPreview());

        y = 0;
        positioning = VerticalPositioning::TOP_BOUND;

        maxWidth = 0;
        for (const auto& paragraphResult : paragraphResults) {
            maxWidth = std::max(maxWidth, paragraphResult.maxLineWidth);
        }

        paragraphResults.clear();

        drawShapes();
    }

    if (paragraphResults.empty()) {
        return TextShapeError::TYPESET_ERROR;
    }

    const auto& p0 = paragraphResults[0];

    if (ctx.config.lastLineDescenderOffset) {
        y = std::round(y - paragraphResults.back().lastlineDescender);
    }

    auto firstLineActualHeight = p0.firstAscender + p0.firstDescender;

    auto w = 0.0f;
    auto h = std::max(std::ceil(y), std::round(ctx.config.preferRealLineHeightOverExplicit ? firstLineActualHeight : p0.firstLineHeight));

    for (const auto& paragraphResult : paragraphResults) {
        w = std::max(w, std::floor(paragraphResult.maxLineWidth));
    }

    auto l = 0.0f;
    auto t = 0.0f;

    if (baselineSet) {
        l -= std::floor(p0.leftFirst);
        t -= std::round(p0.firstAscender);
    }

    preserveFixedDimensions(text.boundsMode(), frameSize, w, h);

    auto textBoundsNoTransform = compat::FRectangle{l, t, w, h};
    auto textBoundsTransform = transform(textBoundsNoTransform, textTransform);

    auto baseline = resolveBaselinePosition(text, p0);

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

TextDrawResult drawText(Context* ctx,
                    const TextShapeData& shapedData,
                    void* pixels, int width, int height,
                    float scale,
                    bool dry,
                    const compat::Rectangle& viewArea)
{
    auto textTransform = shapedData.textTransform;

    auto T_inv = inverse(textTransform);

    auto viewAreaTextSpace = utils::transform(toFRectangle(viewArea), T_inv);
    viewAreaTextSpace = scaleRect(viewAreaTextSpace, scale);

    utils::printRect("view area in text space:", viewAreaTextSpace);

    auto drawResult = drawTextInner(
            *ctx,
            dry,
            *shapedData.formattedText.get(),
            shapedData.textBoundsNoTransform,
            shapedData.baseline,
            scale, viewAreaTextSpace, false,
            shapedData.paragraphShapes,
            static_cast<Pixel32*>(pixels), width, height);

    if (drawResult) {
        auto value = drawResult.moveValue();
        value.transform = shapedData.textTransform;
        value.transform[2][0] *= scale;
        value.transform[2][1] *= scale;
        return value;
    } else {
        return drawResult.error();
    }
}

TextDrawResult drawTextInner(
                    Context& ctx,
                    bool dry,

                    const FormattedText& text,
                    const compat::FRectangle& unscaledTextBounds,
                    float baseline,

                    RenderScale scale,
                    const compat::FRectangle& viewArea,
                    bool alphaMask,

                    const ParagraphShapes& paragraphShapes,
                    compat::Pixel32* pixels,
                    int width,
                    int height
                    )
{

    if (text.getLength() == 0) {
        return TextDrawOutput{{}};
    }

    compat::BitmapRGBA output(compat::BitmapRGBA::WRAP_NO_OWN, pixels, width, height);
    RenderScale origScale = scale;

    auto textBounds = scaleRect(unscaledTextBounds, scale);
    if (!textBounds) {
        return TextDrawError::INVALID_SCALE;
    }

    std::vector<ParagraphShape::DrawResult> paragraphResults;

    auto positioning = VerticalPositioning::BASELINE;
    auto caretVerticalPos = roundCaretPosition(baseline * scale, ctx.config.floorBaseline);
    const auto& shapes = paragraphShapes;
    auto hasBaselineTransform = text.baselinePolicy() == BaselinePolicy::SET;

    // TODO: move to a function
    for (const auto& paragraphShape : shapes) {
        auto last = paragraphShape == shapes.back();
        auto drawResult = paragraphShape->draw(ctx, 0, textBounds.w, caretVerticalPos, positioning, scale, last, hasBaselineTransform, alphaMask);

        const auto lastLinePolicy = (ctx.config.cutLastLine && (shapes.size() > 1 || drawResult.journal.size() > 1))
                                        ? LastLinePolicy::CUT
                                        : LastLinePolicy::FORCE;
        drawResult.journal.setLastLinePolicy(lastLinePolicy);

        paragraphResults.emplace_back(std::move(drawResult));
    }

    if (paragraphResults.empty()) {
        return TextDrawError::PARAGRAPHS_TYPESETING_ERROR;
    }

    // account for descenders of last paragraph's last line
    auto textBottom = caretVerticalPos - paragraphResults.back().lastlineDescender;

    auto baselineOffset = resolveBaselineOffset(text, paragraphResults.front());
    auto verticalOffset = resolveVerticalAlignment(text, textBounds, textBottom, baselineOffset * scale);

    auto unlimitedVerticalStretch = ctx.config.infiniteVerticalStretch || text.boundsMode() == BoundsMode::AUTO_HEIGHT || text.verticalAlign() != VerticalAlign::TOP;

    auto verticalStretchLimit = unlimitedVerticalStretch ? 0 : textBounds.h;
    auto stretchedGlyphsBounds = stretchedBounds(paragraphResults, int(verticalOffset), verticalStretchLimit);
    auto stretchedTextBounds = stretchBounds(textBounds, stretchedGlyphsBounds);

    auto bitmapBounds = outerRect(stretchedTextBounds);

    if (!bitmapBounds) {
        return TextDrawError::DRAW_BOUNDS_ERROR;
    }

    compat::Vector2i offset = {0, 0};

    compat::Rectangle viewAreaBounds = compat::INFINITE_BOUNDS;

    if (ctx.config.enableViewAreaCutout) {
        bitmapBounds = outerRect(viewArea & stretchedTextBounds);
        viewAreaBounds = outerRect(viewArea);

        auto offsetBounds = outerRect(stretchedTextBounds) - bitmapBounds;
        offset = compat::Vector2i{offsetBounds.l, offsetBounds.t};
    }

    if (dry) {
        return TextDrawOutput{bitmapBounds};
    }

    // vertical align offset
    offset.y += int(verticalOffset - stretchedGlyphsBounds.t);

    for (const auto& paragraphResult : paragraphResults) {
        auto journalDrawResult = paragraphResult.journal.draw(output, bitmapBounds, stretchedTextBounds.h, viewAreaBounds, offset);

        if (journalDrawResult) {
            // totalGlyphsDrawn += (decltype(totalGlyphsDrawn)) journalDrawResult.value().numGlyphsRendered;
            // totalGlyphBlitTime += journalDrawResult.value().glyphsRenderTime.timeSpan();
        }
    }

    return TextDrawOutput{bitmapBounds};
}

}
}
