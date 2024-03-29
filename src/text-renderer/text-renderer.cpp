
#include "text-renderer.h"

#include "Context.h"

#include "errors.h"
#include "FormattedParagraph.h"
#include "FormattedText.h"
#include "FreetypeHandle.h"
#include "ParagraphShape.h"
#include "TextShape.h"
#include "TextShapeInput.h"
#include "text-format.h"
#include "TextParser.h"
#include "types.h"
#include "PlacedTextRendering.h"

#include "../compat/affine-transform.h"
#include "../compat/basic-types.h"
#include "../compat/Bitmap.hpp"

#include "../utils/utils.h"

#include <octopus/text.h>

#include <cmath>
#include <cstdio>
#include <iterator>
#include <optional>

namespace odtr {

namespace {
FRectangle convertRect(const compat::FRectangle& r) {
    return FRectangle{r.l, r.t, r.w, r.h};
}
Matrix3f convertMatrix(const compat::Matrix3f &m) {
    return Matrix3f {
        m.m[0][0], m.m[0][1], m.m[0][2],
        m.m[1][0], m.m[1][1], m.m[1][2],
        m.m[2][0], m.m[2][1], m.m[2][2]
    };
}
compat::Matrix3f convertMatrix(const Matrix3f &m) {
    return compat::Matrix3f {
        m.m[0][0], m.m[0][1], m.m[0][2],
        m.m[1][0], m.m[1][1], m.m[1][2],
        m.m[2][0], m.m[2][1], m.m[2][2]
    };
}
}

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
    if (textFormat.empty()) {
        return paragraphs;
    }

    const compat::qchar* textPtr = text.getText();
    const ImmediateFormat* formatPtr = textFormat.data();

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

/// Compute drawn bitmap boundaries as an intersection of the scaled text bounds and the view area.
compat::Rectangle computeDrawBounds(Context &ctx,
                                    const compat::FRectangle &stretchedTextBounds,
                                    const compat::FRectangle& viewAreaTextSpace) {
    return ctx.config.enableViewAreaCutout
        ? utils::outerRect(viewAreaTextSpace & stretchedTextBounds)
        : utils::outerRect(stretchedTextBounds);
}

TextShapeParagraphsResult shapeTextInner(Context &ctx,
                                         const TextShapeInput &textShapeInput) {
    const utils::Log &log = ctx.getLogger();
    const FormattedText &text = *textShapeInput.formattedText;

    // Split the text into paragraphs
    const FormattedParagraphs paragraphs = splitText(log, text, ctx.getFontManager());
    if (paragraphs.empty()) {
        return std::make_pair(TextShapeError::NO_PARAGRAPHS, ParagraphShape::DrawResults {});
    }

    float maxWidth = text.boundsMode() == BoundsMode::AUTO_WIDTH ? 0.0f : textShapeInput.frameSize.value_or(compat::Vector2f{0,0}).x;

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
        return std::make_pair(TextShapeError::NO_PARAGRAPHS, ParagraphShape::DrawResults {});
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
        return std::make_pair(TextShapeError::NO_PARAGRAPHS, ParagraphShape::DrawResults {});
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

    preserveFixedDimensions(text.boundsMode(), textShapeInput.frameSize, w, h);

    const compat::FRectangle textBoundsNoTransform { l, t, w, h };
    const compat::FRectangle textBoundsTransform = transform(textBoundsNoTransform, textShapeInput.textTransform);

    const float baseline = resolveBaselinePosition(p0, text.baselinePolicy(), text.verticalAlign());

    TextShapeDataPtr tsd = std::make_unique<TextShapeData>(std::move(shapes),
                                                           textBoundsNoTransform,
                                                           textBoundsTransform,
                                                           baseline);
    return std::make_pair(std::move(tsd), std::move(paragraphResults));
}

} // namespace


compat::Rectangle computeDrawBounds(Context &ctx,
                                    const PlacedTextData &placedTextData,
                                    float scale,
                                    const compat::Rectangle &viewArea) {
    const compat::FRectangle stretchedTextBounds {
        placedTextData.textBounds.l,
        placedTextData.textBounds.t,
        placedTextData.textBounds.w * scale,
        placedTextData.textBounds.h * scale };

    const compat::FRectangle viewAreaTextSpace = utils::scaleRect(utils::toFRectangle(viewArea), scale);
    return computeDrawBounds(ctx, stretchedTextBounds, viewAreaTextSpace);
}

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

TextShapeInputPtr preprocessText(Context &ctx,
                                 const octopus::Text &text) {
    TextParser::ParseResult parsedText = TextParser(text).parseText();

    FrameSizeOpt frameSize;
    if (text.frame.has_value()) {
        auto [w, h] = text.frame.value().size.value_or(octopus::Dimensions{0.0,0.0});
        frameSize = compat::Vector2f{static_cast<float>(w), static_cast<float>(h)};
    }

    return std::make_unique<TextShapeInput>(std::move(parsedText.text), frameSize, parsedText.transformation);
}

TextShapeResult shapeText(Context &ctx,
                          const TextShapeInput &textShapeInput) {
    TextShapeParagraphsResult res = shapeTextInner(ctx, textShapeInput);
    return std::move(res.first);
}

TextDrawResult drawText(Context &ctx,
                        const TextShapeInput &shapeInput,
                        const TextShapeData &shapeData,
                        float scale,
                        const compat::Rectangle& viewArea,
                        void* pixels, int width, int height,
                        bool dry) {
     if (shapeInput.formattedText == nullptr || shapeInput.formattedText->getLength() == 0) {
         return TextDrawOutput{{}};
     }

    const compat::Matrix3f inverseTransform = inverse(shapeInput.textTransform);
    const compat::FRectangle viewAreaTextSpaceUnscaled = utils::transform(utils::toFRectangle(viewArea), inverseTransform);
    const compat::FRectangle viewAreaTextSpace = utils::scaleRect(viewAreaTextSpaceUnscaled, scale);

    TextDrawResult drawResult = drawTextInner(ctx,
                                              shapeData.paragraphShapes,
                                              shapeInput.formattedText->formattingParams(),
                                              shapeData.textBoundsNoTransform,
                                              shapeData.baseline,
                                              scale,
                                              viewAreaTextSpace,
                                              static_cast<Pixel32*>(pixels), width, height,
                                              dry);

    if (drawResult) {
        TextDrawOutput value = drawResult.moveValue();
        value.transform = shapeInput.textTransform;
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

    for (const ParagraphShape::DrawResult &paragraphResult : paragraphResults) {
        paragraphResult.journal.draw(output, bitmapBounds, stretchedTextBounds.h, viewAreaBounds, offset);
    }

    return TextDrawOutput { bitmapBounds };
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

PlacedTextResult shapePlacedText(Context &ctx, const TextShapeInput &textShapeInput)
{
    const TextShapeParagraphsResult res = shapeTextInner(ctx, textShapeInput);
    if (!res.first) {
        ctx.getLogger().error("Text shaping failed with error: {}", errorToString(res.first.error()));
        return TextShapeError::SHAPE_ERROR;
    }

    const TextShapeDataPtr &textShapeData = res.first.value();
    const ParagraphShape::DrawResults &paragraphResults = res.second;
    const FormattedText::FormattingParams &textParams = textShapeInput.formattedText->formattingParams();
    const compat::FRectangle &textBounds = textShapeData->textBoundsNoTransform;

    // account for descenders of last paragraph's last line
    const float caretVerticalPos = roundCaretPosition(textShapeData->baseline, ctx.config.floorBaseline);
    const spacing textBottom = caretVerticalPos - paragraphResults.back().lastlineDescender;

    const spacing baselineOffset = resolveBaselineOffset(paragraphResults.front(), textParams.baselinePolicy, textParams.verticalAlign);
    const float verticalOffset = resolveVerticalOffset(textParams.boundsMode, textParams.verticalAlign, textBounds, textBottom, baselineOffset);

    const bool unlimitedVerticalStretch =
        textParams.overflowPolicy == OverflowPolicy::EXTEND_ALL ||
        textParams.boundsMode == BoundsMode::AUTO_HEIGHT ||
        textParams.verticalAlign != VerticalAlign::TOP;

    const float verticalStretchLimit = unlimitedVerticalStretch ? 0.0f : textBounds.h;
    compat::Rectangle stretchedGlyphsBounds {};
    if (textParams.boundsMode != BoundsMode::FIXED || textParams.overflowPolicy != OverflowPolicy::NO_OVERFLOW) {
        stretchedGlyphsBounds = stretchedBounds(paragraphResults, int(verticalOffset), verticalStretchLimit);
    }

    // Vertical align offset
    const float verticalAlignOffset = verticalOffset - stretchedGlyphsBounds.t;

    const compat::Matrix3f translationMatrix = compat::translationMatrix(compat::Vector2f { textBounds.l, textBounds.t });
    const compat::FRectangle textBoundsCentered { 0.0f, 0.0f, textBounds.w, textBounds.h };
    const compat::FRectangle textBoundsNotScaled = stretchBounds(textBoundsCentered, stretchedGlyphsBounds);
    const Matrix3f transformMatrix = convertMatrix(translationMatrix*textShapeInput.textTransform);

    PlacedGlyphsPerFont placedGlyphs;
    PlacedDecorations placedDecorations;

    size_t glyphIndex = 0;

    const std::vector<compat::qchar> &inText = textShapeInput.formattedText->text();

    for (size_t i = 0; i < paragraphResults.size(); ++i) {
        const ParagraphShapePtr &paragraphShape = textShapeData->paragraphShapes[i];
        const GlyphShapes &glyphShapes = paragraphShape->glyphs();
        const LineSpans &paragraphLineSpans = paragraphShape->lineSpans();

        const ParagraphShape::DrawResult &drawResult = paragraphResults[i];
        const std::vector<TypesetJournal::LineRecord> &linesDrawn = drawResult.journal.getLines();

        if (paragraphLineSpans.size() != linesDrawn.size()) {
            continue;
        }

        for (size_t k = 0; k < linesDrawn.size(); ++k) {
            const LineSpan &lineSpan = paragraphLineSpans[k];
            const TypesetJournal::LineRecord &lineRecord = linesDrawn[k];

            if (lineSpan.size() != lineRecord.glyphJournal_.size()) {
                continue;
            }

            // Properly ordered glyph shapes on the line. Accounting for LTR-RTL combinations and multiple visual runs
            std::vector<const GlyphShape *> glyphShapesOnLine(lineSpan.size());
            long gsi = 0;
            for (const VisualRun &vr : lineSpan.visualRuns) {
                for (long vri = vr.start; vri < vr.end; ++vri) {
                    glyphShapesOnLine[gsi++] = &glyphShapes[vri];
                }
            }

            // Add placed glyphs
            for (size_t li = 0; li < lineRecord.glyphJournal_.size(); ++li) {
                const GlyphShape *glyphShape = glyphShapesOnLine[li];
                const GlyphPtr &glyph = lineRecord.glyphJournal_[li];

                // Should not happen
                if (glyphShape == nullptr) {
                    ++glyphIndex;
                    continue;
                }

                // Skips glyphs with empty bounds - empty spaces
                if (!glyph->getBitmapBounds()) {
                    ++glyphIndex;
                    continue;
                }

                // Find the glyph index
                for (size_t gi = glyphIndex; gi < inText.size(); ++gi) {
                    if (inText[gi] != glyphShape->character) {
                        ++glyphIndex;
                    } else {
                        break;
                    }
                }

                PlacedGlyphs &placedGlyphsForFont = placedGlyphs[FontSpecifier { glyphShape->format.faceId }];
                placedGlyphsForFont.emplace_back();
                PlacedGlyph &placedGlyph = placedGlyphsForFont.back();

                placedGlyph.codepoint = glyphShape->codepoint;
                placedGlyph.color = glyphShape->format.color;
                placedGlyph.fontSize = glyphShape->format.size;
                placedGlyph.index = glyphIndex;
                placedGlyph.originPosition = Vector2f {
                    glyph->getOrigin().x,
                    glyph->getOrigin().y + verticalAlignOffset,
                };

                ++glyphIndex;
            }

            // Add placed decorations
            for (const TypesetJournal::DecorationRecord &decoration : lineRecord.decorationJournal_) {
                placedDecorations.emplace_back();
                PlacedDecoration &placedDecoration = placedDecorations.back();

                placedDecoration.type = static_cast<PlacedDecoration::Type>(decoration.type);
                placedDecoration.color = decoration.color;

                placedDecoration.start = Vector2f {
                    decoration.range.first,
                    decoration.offset + verticalAlignOffset,
                };
                placedDecoration.end = Vector2f {
                    decoration.range.last,
                    decoration.offset + verticalAlignOffset,
                };
                placedDecoration.thickness = decoration.thickness;
            }
        }
    }

    return std::make_unique<PlacedTextData>(std::move(placedGlyphs),
                                            std::move(placedDecorations),
                                            convertRect(textBoundsNotScaled),
                                            transformMatrix);
}

TextDrawResult drawPlacedText(Context &ctx,
                              const PlacedTextData &placedTextData,
                              float scale,
                              const compat::Rectangle &viewArea,
                              void *pixels, int width, int height) {
    const compat::FRectangle viewAreaTextSpace = utils::scaleRect(utils::toFRectangle(viewArea), scale);

    TextDrawResult drawResult = drawPlacedTextInner(ctx,
                                                    placedTextData,
                                                    scale,
                                                    viewAreaTextSpace,
                                                    static_cast<Pixel32*>(pixels), width, height);

    if (drawResult) {
        const compat::FRectangle stretchedTextBounds {
            placedTextData.textBounds.l * scale,
            placedTextData.textBounds.t * scale,
            placedTextData.textBounds.w * scale,
            placedTextData.textBounds.h * scale };

        TextDrawOutput value = drawResult.moveValue();
        value.transform = convertMatrix(placedTextData.textTransform);
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

    const compat::Rectangle viewAreaBounds = (ctx.config.enableViewAreaCutout) ? utils::outerRect(viewArea) : compat::INFINITE_BOUNDS;

    for (const auto &pgIt : placedTextData.glyphs) {
        const FontSpecifier &fontSpecifier = pgIt.first;
        const PlacedGlyphs &placedGlyphs = pgIt.second;

        const FaceTable::Item* faceItem = ctx.getFontManager().facesTable().getFaceItem(fontSpecifier.faceId);
        if (faceItem != nullptr && faceItem->face != nullptr) {
            for (const PlacedGlyph &pg : placedGlyphs) {
                const GlyphPtr renderedGlyph = renderPlacedGlyph(pg,
                                                                 faceItem->face,
                                                                 scale,
                                                                 ctx.config.internalDisableHinting);

                if (renderedGlyph != nullptr) {
                    drawGlyph(output, *renderedGlyph, viewAreaBounds);
                }
            }
        }
    }

    for (const PlacedDecoration &pd : placedTextData.decorations) {
        drawDecoration(output, pd, scale);
    }

    return TextDrawOutput{};
}

} // namespace priv
} // namespace odtr
