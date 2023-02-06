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
#include "PlacedTextRendering.h"

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

    for (const ParagraphShape::DrawResult &result : paragraphResults) {
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
        const int parLen = FormattedParagraph::extractParagraph(paragraphs.back(), textPtr, formatPtr, limit);

        textPtr += parLen;
        formatPtr += parLen;
        limit -= parLen;
    }

    for (FormattedParagraph &p : paragraphs) {
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
    TextParser::ParseResult parsedText = TextParser(text).parseText();

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

    ParagraphShape::DrawResults paragraphResults = drawParagraphsInner(ctx,
                                                                       shapes,
                                                                       text.overflowPolicy(),
                                                                       static_cast<int>(std::floor(maxWidth)),
                                                                       1.0f,
                                                                       VerticalPositioning::TOP_BOUND,
                                                                       text.baselinePolicy(),
                                                                       y);

    // Rerun glyph bitmaps if previous justification was nonsense (zero width for auto-width bounds)
    if (text.boundsMode() == BoundsMode::AUTO_WIDTH) {
        log.debug("Running second pass for text '{}", text.getPreview());

        y = 0.0f;
        maxWidth = 0.0f;
        for (const auto& paragraphResult : paragraphResults) {
            maxWidth = std::max(maxWidth, paragraphResult.maxLineWidth);
        }

        paragraphResults = drawParagraphsInner(ctx,
                                               shapes,
                                               text.overflowPolicy(),
                                               static_cast<int>(std::floor(maxWidth)),
                                               1.0f,
                                               VerticalPositioning::TOP_BOUND,
                                               text.baselinePolicy(),
                                               y);
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
                        float scale,
                        const compat::Rectangle& viewArea,
                        void* pixels, int width, int height,
                        bool dry) {
     if (shapeData.formattedText == nullptr || shapeData.formattedText->getLength() == 0) {
         return TextDrawOutput{{}};
     }

    const compat::Matrix3f inverseTransform = inverse(shapeData.textTransform);
    const compat::FRectangle viewAreaTextSpaceUnscaled = utils::transform(utils::toFRectangle(viewArea), inverseTransform);
    const compat::FRectangle viewAreaTextSpace = utils::scaleRect(viewAreaTextSpaceUnscaled, scale);

    TextDrawResult drawResult = drawTextInner(ctx,
                                              shapeData.paragraphShapes,
                                              shapeData.formattedText->formattingParams(),
                                              shapeData.textBoundsNoTransform,
                                              shapeData.baseline,
                                              scale,
                                              viewAreaTextSpace,
                                              static_cast<Pixel32*>(pixels), width, height,
                                              dry);

    if (drawResult) {
        TextDrawOutput value = drawResult.moveValue();
        value.transform = shapeData.textTransform;
        value.transform[2][0] *= scale;
        value.transform[2][1] *= scale;
        return value;
    } else {
        return drawResult.error();
    }
}

TextDrawResult drawTextInner(Context &ctx,
                             const ParagraphShapes& paragraphShapes,
                             const FormattedText::FormattingParams &textParams,
                             const compat::FRectangle& unscaledTextBounds,
                             float baseline,
                             RenderScale scale, const compat::FRectangle& viewArea,
                             Pixel32* pixels, int width, int height,
                             bool dry) {
    const compat::FRectangle textBounds = utils::scaleRect(unscaledTextBounds, scale);
    if (!textBounds) {
        return TextDrawError::INVALID_SCALE;
    }

    float caretVerticalPos = roundCaretPosition(baseline * scale, ctx.config.floorBaseline);

    const ParagraphShape::DrawResults paragraphResults = drawParagraphsInner(ctx,
                                                                             paragraphShapes,
                                                                             textParams.overflowPolicy,
                                                                             textBounds.w,
                                                                             scale,
                                                                             VerticalPositioning::BASELINE,
                                                                             textParams.baselinePolicy,
                                                                             caretVerticalPos);
    if (paragraphResults.empty()) {
        return TextDrawError::PARAGRAPHS_TYPESETING_ERROR;
    }

    // account for descenders of last paragraph's last line
    const spacing textBottom = caretVerticalPos - paragraphResults.back().lastlineDescender;

    const spacing baselineOffset = resolveBaselineOffset(paragraphResults.front(), textParams.baselinePolicy, textParams.verticalAlign);
    const float verticalOffset = resolveVerticalOffset(textParams.boundsMode, textParams.verticalAlign, textBounds, textBottom, baselineOffset * scale);

    const bool unlimitedVerticalStretch =
        textParams.overflowPolicy == OverflowPolicy::EXTEND_ALL ||
        textParams.boundsMode == BoundsMode::AUTO_HEIGHT ||
        textParams.verticalAlign != VerticalAlign::TOP;

    const float verticalStretchLimit = unlimitedVerticalStretch ? 0.0f : textBounds.h;
    compat::Rectangle stretchedGlyphsBounds {};
    if (textParams.boundsMode != BoundsMode::FIXED || textParams.overflowPolicy != OverflowPolicy::NO_OVERFLOW) {
        stretchedGlyphsBounds = stretchedBounds(paragraphResults, int(verticalOffset), verticalStretchLimit);
    }
    const compat::FRectangle stretchedTextBounds = stretchBounds(textBounds, stretchedGlyphsBounds);

    compat::Rectangle bitmapBounds = utils::outerRect(stretchedTextBounds);

    if (!bitmapBounds) {
        return TextDrawError::DRAW_BOUNDS_ERROR;
    }

    if (ctx.config.enableViewAreaCutout) {
        bitmapBounds = utils::outerRect(viewArea & stretchedTextBounds);
    }

    if (dry) {
        return TextDrawOutput{bitmapBounds};
    }

    compat::Vector2i offset = {0, 0};
    compat::Rectangle viewAreaBounds = compat::INFINITE_BOUNDS;

    if (ctx.config.enableViewAreaCutout) {
        const compat::Rectangle offsetBounds = utils::outerRect(stretchedTextBounds) - bitmapBounds;

        viewAreaBounds = utils::outerRect(viewArea);
        offset = compat::Vector2i{offsetBounds.l, offsetBounds.t};
    }

    // vertical align offset
    offset.y += int(verticalOffset - stretchedGlyphsBounds.t);

    compat::BitmapRGBA output(compat::BitmapRGBA::WRAP_NO_OWN, pixels, width, height);

//    debug_drawBitmapBoundaries(output, width, height);
//    debug_drawBitmapGrid(output, width, height);

    for (const ParagraphShape::DrawResult &paragraphResult : paragraphResults) {
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
                                                OverflowPolicy overflowPolicy,
                                                int textWidth,
                                                RenderScale scale,
                                                VerticalPositioning positioning,
                                                BaselinePolicy baselinePolicy,
                                                float &caretVerticalPos) {
    ParagraphShape::DrawResults drawResults;

    for (const ParagraphShapePtr& paragraphShape : shapes) {
        const bool isLast = (paragraphShape == shapes.back());
        ParagraphShape::DrawResult drawResult = paragraphShape->draw(ctx,
                                                                     0,
                                                                     textWidth,
                                                                     caretVerticalPos,
                                                                     positioning,
                                                                     baselinePolicy,
                                                                     scale,
                                                                     isLast,
                                                                     false);

        const LastLinePolicy lastLinePolicy =
            (overflowPolicy == OverflowPolicy::CLIP_LINE && drawResult.journal.size() > 1)
            ? LastLinePolicy::CUT
            : LastLinePolicy::FORCE;
        drawResult.journal.setLastLinePolicy(lastLinePolicy);

        drawResults.emplace_back(std::move(drawResult));
    }

    return drawResults;
}


// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------------


compat::FRectangle getStretchedTextBounds(Context &ctx,
                                          const ParagraphShapes &paragraphShapes,
                                          const compat::FRectangle &unscaledTextBounds,
                                          const FormattedText::FormattingParams &textParams,
                                          float baseline,
                                          float scale) {
    const compat::FRectangle textBounds = utils::scaleRect(unscaledTextBounds, scale);
    if (!textBounds) {
        return compat::FRectangle{};
    }

    float caretVerticalPos = roundCaretPosition(baseline * scale, ctx.config.floorBaseline);

    const ParagraphShape::DrawResults paragraphResults = drawParagraphsInner(ctx,
                                                                             paragraphShapes,
                                                                             textParams.overflowPolicy,
                                                                             textBounds.w,
                                                                             scale,
                                                                             VerticalPositioning::BASELINE,
                                                                             textParams.baselinePolicy,
                                                                             caretVerticalPos);
    if (paragraphResults.empty()) {
        return compat::FRectangle{};
    }

    // account for descenders of last paragraph's last line
    const spacing textBottom = caretVerticalPos - paragraphResults.back().lastlineDescender;

    const spacing baselineOffset = resolveBaselineOffset(paragraphResults.front(), textParams.baselinePolicy, textParams.verticalAlign);
    const float verticalOffset = resolveVerticalOffset(textParams.boundsMode, textParams.verticalAlign, textBounds, textBottom, baselineOffset * scale);

    const bool unlimitedVerticalStretch =
        textParams.overflowPolicy == OverflowPolicy::EXTEND_ALL ||
        textParams.boundsMode == BoundsMode::AUTO_HEIGHT ||
        textParams.verticalAlign != VerticalAlign::TOP;

    const float verticalStretchLimit = unlimitedVerticalStretch ? 0.0f : textBounds.h;
    compat::Rectangle stretchedGlyphsBounds {};
    if (textParams.boundsMode != BoundsMode::FIXED || textParams.overflowPolicy != OverflowPolicy::NO_OVERFLOW) {
        stretchedGlyphsBounds = stretchedBounds(paragraphResults, int(verticalOffset), verticalStretchLimit);
    }

    return stretchBounds(textBounds, stretchedGlyphsBounds);
}

compat::Rectangle computeDrawBounds(Context &ctx,
                                    const compat::FRectangle &stretchedTextBounds,
                                    const compat::FRectangle& viewAreaTextSpace) {
    return ctx.config.enableViewAreaCutout
        ? utils::outerRect(viewAreaTextSpace & stretchedTextBounds)
        : utils::outerRect(stretchedTextBounds);
}

compat::Rectangle computeDrawBounds(Context &ctx,
                                    const PlacedTextData &placedTextData,
                                    float scale,
                                    const compat::Rectangle &viewArea) {
    const compat::FRectangle stretchedTextBounds = placedTextData.textBounds * scale;
    const compat::FRectangle viewAreaTextSpace = utils::scaleRect(utils::toFRectangle(viewArea), scale);
    return computeDrawBounds(ctx, stretchedTextBounds, viewAreaTextSpace);
}

PlacedTextResult shapePlacedText(Context &ctx,
                               const octopus::Text& text)
{
    TextParser::ParseResult parsedText = TextParser(text).parseText();

    FrameSizeOpt frameSize;
    if (text.frame.has_value()) {
        const auto [w, h] = text.frame.value().size.value_or(octopus::Dimensions{0.0,0.0});
        frameSize = compat::Vector2f{static_cast<float>(w), static_cast<float>(h)};
    }
    return shapePlacedTextInner(ctx, std::move(parsedText.text), frameSize, parsedText.transformation);
}

// TODO: Matus: This function is the same as the old one, the only difference is it collects the PlacedGlyphs.
PlacedTextResult shapePlacedTextInner(Context &ctx,
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
    ParagraphShape::DrawResults paragraphResults = drawParagraphsInner(ctx,
                                                                       shapes,
                                                                       text.overflowPolicy(),
                                                                       static_cast<int>(std::floor(maxWidth)),
                                                                       1.0f,
                                                                       VerticalPositioning::TOP_BOUND,
                                                                       text.baselinePolicy(),
                                                                       y);

    // Rerun glyph bitmaps if previous justification was nonsense (zero width for auto-width bounds)
    if (text.boundsMode() == BoundsMode::AUTO_WIDTH) {
        log.debug("Running second pass for text '{}", text.getPreview());

        y = 0.0f;
        maxWidth = 0.0f;
        for (const ParagraphShape::DrawResult& paragraphResult : paragraphResults) {
            maxWidth = std::max(maxWidth, paragraphResult.maxLineWidth);
        }

        paragraphResults = drawParagraphsInner(ctx,
                                               shapes,
                                               text.overflowPolicy(),
                                               static_cast<int>(std::floor(maxWidth)),
                                               1.0f,
                                               VerticalPositioning::TOP_BOUND,
                                               text.baselinePolicy(),
                                               y);
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

    for (const ParagraphShape::DrawResult& paragraphResult : paragraphResults) {
        w = std::max(w, std::floor(paragraphResult.maxLineWidth));
    }

    const bool isBaselineSet = text.baselinePolicy() == BaselinePolicy::SET;
    const float l = isBaselineSet ? -std::floor(p0.leftFirst) : 0.0f;
    const float t = isBaselineSet ? -std::round(p0.firstAscender) : 0.0f;

    preserveFixedDimensions(text.boundsMode(), frameSize, w, h);

    const compat::FRectangle textBoundsNoTransform { l, t, w, h };
    const compat::FRectangle textBoundsTransformed = transform(textBoundsNoTransform, textTransform);

    const float baseline = resolveBaselinePosition(p0, text.baselinePolicy(), text.verticalAlign());

    PlacedGlyphsPerFont placedGlyphs;
    PlacedDecorations placedDecorations;

    for (size_t i = 0; i < paragraphResults.size(); i++) {
        const ParagraphShapePtr &paragraphShape = shapes[i];
        const ParagraphShape::DrawResult &drawResult = paragraphResults[i];

        size_t j = 0;
        for (size_t k = 0; k < drawResult.journal.getLines().size(); k++) {
            const TypesetJournal::LineRecord &lineRecord = drawResult.journal.getLines()[k];

            for (size_t l = 0; l < lineRecord.glyphJournal_.size(); l++) {
                const GlyphShape &glyphShape = paragraphShape->glyphs()[j];
                const GlyphPtr &glyph = lineRecord.glyphJournal_[l];
                const compat::Vector2f &offset = lineRecord.offsets_[l];

                const IPoint2 &glyphDestination = glyph->getDestination();

                PlacedGlyphPtr placedGlyph = std::make_unique<PlacedGlyph>();

                placedGlyph->glyphCodepoint = glyphShape.codepoint;
                placedGlyph->color = glyphShape.format.color;
                placedGlyph->fontSize = glyphShape.format.size;

                const float bitmapWidthF = static_cast<float>(glyph->bitmapWidth());
                const float bitmapHeightF = static_cast<float>(glyph->bitmapHeight());

                placedGlyph->placement.topLeft = Vector2f {
                    static_cast<float>(glyphDestination.x) + offset.x,
                    static_cast<float>(glyphDestination.y) + offset.y };
                const Vector2f &tl = placedGlyph->placement.topLeft;
                placedGlyph->placement.topRight = Vector2f {
                    tl.x + bitmapWidthF,
                    tl.y };
                placedGlyph->placement.bottomLeft = Vector2f {
                    tl.x,
                    tl.y + bitmapHeightF };
                placedGlyph->placement.bottomRight = Vector2f {
                    tl.x + bitmapWidthF,
                    tl.y + bitmapHeightF };

                placedGlyphs[FontSpecifier { glyphShape.format.faceId }].emplace_back(std::move(placedGlyph));

                j++;
            }

            for (size_t l = 0; l < lineRecord.decorationJournal_.size(); l++) {
                const TypesetJournal::DecorationRecord &decoration = lineRecord.decorationJournal_[l];

                PlacedDecorationPtr placedDecoration = std::make_unique<PlacedDecoration>();

                placedDecoration->type = static_cast<PlacedDecoration::Type>(decoration.type);
                placedDecoration->color = decoration.color;
                placedDecoration->placement.xFirst = decoration.range.first;
                placedDecoration->placement.xLast = decoration.range.last;
                placedDecoration->placement.y = decoration.offset;
                placedDecoration->thickness = decoration.thickness;

                placedDecorations.emplace_back(std::move(placedDecoration));
            }
        }
    }

    // Compute the unscaled stretched bounds
    const compat::Vector2f textTranslation { textTransform.m[2][0], textTransform.m[2][1] };
    compat::FRectangle unstretchedTextBounds =
        getStretchedTextBounds(ctx, shapes, textBoundsNoTransform, formattedText->formattingParams(), baseline, 1.0f) +
        textTranslation;

    return std::make_unique<PlacedTextData>(std::move(placedGlyphs),
                                            std::move(placedDecorations),
                                            unstretchedTextBounds);
}

TextDrawResult drawPlacedText(Context &ctx,
                              const PlacedTextData &placedTextData,
                              float scale,
                              const compat::Rectangle &viewArea,
                              void *pixels, int width, int height) {
    const compat::FRectangle stretchedTextBounds = placedTextData.textBounds * scale;

    const compat::FRectangle viewAreaTextSpace = utils::scaleRect(utils::toFRectangle(viewArea), scale);

    TextDrawResult drawResult = drawPlacedTextInner(ctx,
                                                    placedTextData,
                                                    scale,
                                                    viewAreaTextSpace,
                                                    static_cast<Pixel32*>(pixels), width, height);

    if (drawResult) {
        TextDrawOutput value = drawResult.moveValue();
        value.transform = compat::Matrix3f::identity;
        value.drawBounds = computeDrawBounds(ctx, stretchedTextBounds, viewAreaTextSpace);
        return value;
    } else {
        return drawResult.error();
    }
}

TextDrawResult drawPlacedTextInner(Context &ctx,
                                   const PlacedTextData &placedTextData,
                                   RenderScale scale,
                                   const compat::FRectangle& viewArea,
                                   Pixel32* pixels, int width, int height) {
    compat::BitmapRGBA output(compat::BitmapRGBA::WRAP_NO_OWN, pixels, width, height);

//    debug_drawBitmapBoundaries(output, width, height);
//    debug_drawBitmapGrid(output, width, height);

    const compat::Rectangle viewAreaBounds = (ctx.config.enableViewAreaCutout) ? utils::outerRect(viewArea) : compat::INFINITE_BOUNDS;

    for (const auto &pgIt : placedTextData.glyphs) {
        const FontSpecifier &fontSpecifier = pgIt.first;
        const PlacedGlyphs &placedGlyphs = pgIt.second;

        const FaceTable::Item* faceItem = ctx.getFontManager().facesTable().getFaceItem(fontSpecifier.faceId);
        if (faceItem != nullptr && faceItem->face != nullptr) {
            for (const PlacedGlyphPtr &pg : placedGlyphs) {
                const GlyphPtr renderedGlyph = renderPlacedGlyph(*pg,
                                                                 faceItem->face,
                                                                 scale,
                                                                 ctx.config.internalDisableHinting);

                if (renderedGlyph != nullptr) {
                    drawGlyph(output, *renderedGlyph, viewAreaBounds);
        //            debug_drawGlyphBoundingRectangle(output, *pg);
                }
            }
        }
    }

    for (const PlacedDecorationPtr& pd : placedTextData.decorations) {
        drawDecoration(output, *pd, scale);
    }

    return TextDrawOutput{};
}

} // namespace priv
} // namespace textify
