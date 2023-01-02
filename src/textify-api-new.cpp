
#include "textify/textify-api-new.h"

#include "textify/textify-api.h"

#include "compat/affine-transform.h"
#include "compat/basic-types.h"

#include "fonts/FontManager.h"

#include "textify/Config.h"
#include "textify/Context.h"
#include "textify/textify.h"
#include "textify/TextShape.h"
#include "textify/types.h"

#include "utils/utils.h"
#include "utils/Log.h"
#include "utils/fmt.h"

#include "vendor/fmt/core.h"

#include "textify/text-format.h"

#include <octopus/text.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace textify {
namespace {
compat::Rectangle convertRect(const textify::Rectangle &r) {
    return compat::Rectangle{r.l, r.t, r.w, r.h};
}
FRectangle convertRect(const compat::FRectangle &r) {
    return FRectangle{r.l, r.t, r.w, r.h};
}
compat::FRectangle convertRect(const FRectangle &r) {
    return compat::FRectangle{r.l, r.t, r.w, r.h};
}
Matrix3f convertMatrix(const compat::Matrix3f &m) {
    return Matrix3f { m.m[0][0], m.m[0][1], m.m[0][2], m.m[1][0], m.m[1][1], m.m[1][2], m.m[2][0], m.m[2][1], m.m[2][2] };
}
compat::Matrix3f convertMatrix(const Matrix3f &m) {
    return compat::Matrix3f { m.m[0][0], m.m[0][1], m.m[0][2], m.m[1][0], m.m[1][1], m.m[1][2], m.m[2][0], m.m[2][1], m.m[2][2] };
}
}

PlacedGlyph convertToPlacedGlyph(const priv::GlyphShape &glyph) {
    const PlacedGlyph::Temporary::Dimensions dimensions {
        glyph.horizontalAdvance,
        glyph.ascender,
        glyph.descender,
        glyph.bearingX,
        glyph.bearingY,
        glyph.lineHeight,
    };

    PlacedGlyph::Temporary temp;

    temp.format.size = glyph.format.size;

    temp.format.paragraphSpacing = glyph.format.paragraphSpacing;
    temp.format.decoration = static_cast<Decoration_NEW>(glyph.format.decoration);
    temp.format.align = static_cast<HorizontalAlign_NEW>(glyph.format.align);

    temp.fontFaceId = glyph.format.faceId;
    temp.defaultLineHeight = glyph.defaultLineHeight;

    temp.dimensions.horizontalAdvance = glyph.horizontalAdvance;
    temp.dimensions.ascender = glyph.ascender;
    temp.dimensions.descender = glyph.descender;
    temp.dimensions.bearingX = glyph.bearingX;
    temp.dimensions.bearingY = glyph.bearingY;
    temp.dimensions.lineHeight = glyph.lineHeight;

    PlacedGlyph result;

    result.glyphCodepoint = glyph.codepoint;
    result.quadCorners = {};
    result.color = glyph.format.color;
    result.temp = temp;

    return result;
}

priv::GlyphShape convertToGlyphShape(/*const FontSpecifier &fontSpecifier,*/
                                     const PlacedGlyph &placedGlyph) {
    priv::GlyphShape result;

    ImmediateFormat resultFormat;

    // TODO: Matus: Many of the result glyph (or format) parameters are not set - not needed in the rendering phase
    resultFormat.faceId = placedGlyph.temp.fontFaceId;
    resultFormat.size = placedGlyph.temp.format.size;
    resultFormat.paragraphSpacing = placedGlyph.temp.format.paragraphSpacing;
    resultFormat.color = placedGlyph.color;
    resultFormat.decoration = static_cast<Decoration>(placedGlyph.temp.format.decoration);
    resultFormat.align = static_cast<HorizontalAlign>(placedGlyph.temp.format.align);

    result.format = resultFormat;
    result.codepoint = placedGlyph.glyphCodepoint;

    result.defaultLineHeight = placedGlyph.temp.defaultLineHeight;
    result.horizontalAdvance = placedGlyph.temp.dimensions.horizontalAdvance;
    result.ascender = placedGlyph.temp.dimensions.ascender;
    result.descender = placedGlyph.temp.dimensions.descender;
    result.bearingX = placedGlyph.temp.dimensions.bearingX;
    result.bearingY = placedGlyph.temp.dimensions.bearingY;
    result.lineHeight = placedGlyph.temp.dimensions.lineHeight;

    return result;
}

VisualRun_NEW convertToVisualRun_NEW(const VisualRun &visualRun) {
    VisualRun_NEW result;

    result.start = visualRun.start;
    result.end = visualRun.end;
    result.width = visualRun.width;
    result.leftToRight = visualRun.dir == TextDirection::LEFT_TO_RIGHT;

    return result;
}

Line_NEW convertToLineSpan_NEW(const priv::LineSpan &lineSpan) {
    Line_NEW result;

    result.start = lineSpan.start;
    result.end = lineSpan.end;

    for (const VisualRun &visualRun : lineSpan.visualRuns) {
        result.visualRuns.emplace_back(convertToVisualRun_NEW(visualRun));
    }

    result.lineWidth = lineSpan.lineWidth;
    result.baseDirLeftToRight = lineSpan.baseDir == TextDirection::LEFT_TO_RIGHT;
    result.justifiable = static_cast<Justifiable_NEW>(lineSpan.justifiable);

    return result;
}

VisualRun convertToVisualRun(const VisualRun_NEW &visualRun) {
    VisualRun result;

    result.start = visualRun.start;
    result.end = visualRun.end;
    result.width = visualRun.width;
    result.dir = visualRun.leftToRight ? TextDirection::LEFT_TO_RIGHT : TextDirection::RIGHT_TO_LEFT;

    return result;
}

priv::LineSpan convertToLineSpan(const Line_NEW &lineSpan) {
    priv::LineSpan result;

    result.start = lineSpan.start;
    result.end = lineSpan.end;

    for (const VisualRun_NEW &visualRun : lineSpan.visualRuns) {
        result.visualRuns.emplace_back(convertToVisualRun(visualRun));
    }

    result.lineWidth = lineSpan.lineWidth;
    result.baseDir = lineSpan. baseDirLeftToRight ? TextDirection::LEFT_TO_RIGHT :  TextDirection::RIGHT_TO_LEFT;
    result.justifiable = static_cast<priv::LineSpan::Justifiable>(lineSpan.justifiable);

    return result;
}


ShapeTextResult_NEW shapeText_NEW(ContextHandle ctx,
                                  const octopus::Text &text) {
    ShapeTextResult_NEW result;
    if (ctx == nullptr) {
        return result;
    }

    priv::TextShapeResult textShapeResult = priv::shapeText(*ctx, text);
    if (!textShapeResult) {
        ctx->getLogger().error("shaping of a text failed with error: {}", (int)textShapeResult.error());
        return result;
    }

    const priv::TextShapeDataPtr textShapeData = textShapeResult.moveValue();
    if (textShapeData == nullptr) {
        return result;
    }

    result.textTransform = convertMatrix(textShapeData->textTransform);
    result.textBoundsNoTransform = convertRect(textShapeData->textBoundsNoTransform);
    result.textParams.baseline = textShapeData->baseline;
    result.textParams.verticalAlign = static_cast<ShapeTextResult_NEW::TextParams::VerticalAlign>(textShapeData->formattedText->verticalAlign());
    result.textParams.boundsMode = static_cast<ShapeTextResult_NEW::TextParams::BoundsMode>(textShapeData->formattedText->boundsMode());
    result.textParams.baselinePolicy = static_cast<ShapeTextResult_NEW::TextParams::BaselinePolicy>(textShapeData->formattedText->baselinePolicy());
    result.textParams.overflowPolicy = static_cast<ShapeTextResult_NEW::TextParams::OverflowPolicy>(textShapeData->formattedText->overflowPolicy());

    for (const priv::ParagraphShapePtr &paragraphShape : textShapeData->paragraphShapes) {
        if (paragraphShape == nullptr)
            continue;

        Paragraph_NEW &pNew = result.paragraphs.emplace_back();
        for (const priv::GlyphShape &glyph : paragraphShape->glyphs()) {
            pNew.glyphs_.emplace_back(convertToPlacedGlyph(glyph));
        }
        for (const priv::LineSpan &lineSpan : paragraphShape->lineSpans()) {
            pNew.lines_.emplace_back(convertToLineSpan_NEW(lineSpan));
        }
    }

    return result;
}

DrawTextResult drawText_NEW(ContextHandle ctx,
                            const ShapeTextResult_NEW &textShape_NEW,
                            void *outputBuffer, int width, int height,
                            const DrawOptions &drawOptions)
{
    if (ctx == nullptr) {
        return {};
    }

    priv::ParagraphShapes paragraphShapes;

    for (const Paragraph_NEW &paragraphShape : textShape_NEW.paragraphs) {
        priv::ParagraphShapePtr &paragraph = paragraphShapes.emplace_back(std::make_unique<priv::ParagraphShape>(ctx->getLogger(), ctx->getFontManager().facesTable()));

        priv::GlyphShapes glyphs;
        priv::LineSpans lineSpans;

        for (const PlacedGlyph &placedGlyph : paragraphShape.glyphs_) {
            glyphs.emplace_back(convertToGlyphShape(placedGlyph));
        }
        for (const Line_NEW &lineNew : paragraphShape.lines_) {
            lineSpans.emplace_back(convertToLineSpan(lineNew));
        }

        paragraph->initialize(glyphs, lineSpans);
    }

//    if (textShape && sanitizeShape(ctx, textShape)) {

    const compat::Rectangle viewArea = drawOptions.viewArea.has_value()
        ? convertRect(drawOptions.viewArea.value())
        : compat::INFINITE_BOUNDS;

    const priv::FormattedText::FormattingParams formattedTextParams {
        static_cast<VerticalAlign>(textShape_NEW.textParams.verticalAlign),
        static_cast<BoundsMode>(textShape_NEW.textParams.boundsMode),
        textShape_NEW.textParams.baseline,
        static_cast<HorizontalPositionPolicy>(textShape_NEW.textParams.horizontalPolicy), // TODO: Matus: This does not get set and is unused. Have a closer look!
        static_cast<BaselinePolicy>(textShape_NEW.textParams.baselinePolicy),
        static_cast<OverflowPolicy>(textShape_NEW.textParams.overflowPolicy),
    };

    const compat::Matrix3f textTransform = convertMatrix(textShape_NEW.textTransform);
    const compat::FRectangle textBoundsNoTransform = convertRect(textShape_NEW.textBoundsNoTransform);
    const priv::TextDrawResult result = priv::drawText(*ctx,
                                                       paragraphShapes,
                                                       textTransform,
                                                       textBoundsNoTransform,
                                                       formattedTextParams,
                                                       textShape_NEW.textParams.baseline,
                                                       outputBuffer, width, height,
                                                       drawOptions.scale,
                                                       false,
                                                       viewArea);
    if (result) {
        const auto& drawOutput = result.value();
        return {
            utils::castRectangle(drawOutput.drawBounds), utils::castMatrix(drawOutput.transform),
            false
        };
    }

//    }

    return DrawTextResult {};
}

} // namespace textify
