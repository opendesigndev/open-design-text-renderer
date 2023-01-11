
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
static std::map<TextShapeHandle, ShapeTextResult_NEW> results_NEW;
}

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
    PlacedGlyph::Temporary temp;

    temp.format.size = glyph.format.size;

    temp.format.paragraphSpacing = glyph.format.paragraphSpacing;
    temp.format.decoration = static_cast<Decoration_NEW>(glyph.format.decoration);
    temp.format.align = static_cast<HorizontalAlign_NEW>(glyph.format.align);

    temp.defaultLineHeight = glyph.defaultLineHeight;

    temp.dimensions = {
        glyph.ascender,
        glyph.descender,
        glyph.lineHeight,
    };

    PlacedGlyph result;

    result.glyphCodepoint = glyph.codepoint;
    result.quadCorners = {};
    result.color = glyph.format.color;
    result.fontFaceId = glyph.format.faceId;
    result.temp = temp;

    return result;
}

priv::GlyphShape convertToGlyphShape(/*const FontSpecifier &fontSpecifier,*/
                                     const PlacedGlyph &placedGlyph) {
    priv::GlyphShape result;

    ImmediateFormat resultFormat;

    // TODO: Matus: Many of the result glyph (or format) parameters are not set - not needed in the rendering phase
    resultFormat.faceId = placedGlyph.fontFaceId;
    resultFormat.size = placedGlyph.temp.format.size;
    resultFormat.paragraphSpacing = placedGlyph.temp.format.paragraphSpacing;
    resultFormat.color = placedGlyph.color;
    resultFormat.decoration = static_cast<Decoration>(placedGlyph.temp.format.decoration);
    resultFormat.align = static_cast<HorizontalAlign>(placedGlyph.temp.format.align);

    result.format = resultFormat;
    result.codepoint = placedGlyph.glyphCodepoint;

    result.defaultLineHeight = placedGlyph.temp.defaultLineHeight;
    result.ascender = placedGlyph.temp.dimensions.ascender;
    result.descender = placedGlyph.temp.dimensions.descender;
    result.lineHeight = placedGlyph.temp.dimensions.lineHeight;

    return result;
}


TextShapeHandle shapeText_NEW(ContextHandle ctx,
                              const octopus::Text& text)
{
    if (ctx == nullptr) {
        return nullptr;
    }

    priv::TextShapeResult textShapeResult = priv::shapeText(*ctx, text);
    if (!textShapeResult) {
        ctx->getLogger().error("shaping of a text failed with error: {}", (int)textShapeResult.error());
        return nullptr;
    }

    ctx->shapes.emplace_back(std::make_unique<TextShape>(textShapeResult.moveValue()));

    results_NEW[ctx->shapes.back().get()] = shapeText_NEW_Inner(ctx, text);

    return ctx->shapes.back().get();
}

ShapeTextResult_NEW shapeText_NEW_Inner(ContextHandle ctx,
                                        const octopus::Text &text) {
    ShapeTextResult_NEW result;
    if (ctx == nullptr) {
        return result;
    }

    priv::TextShapeResult_NEW textShapeResult = priv::shapeText_NEW(*ctx, text);
    if (!textShapeResult) {
        ctx->getLogger().error("shaping of a text failed with error: {}", (int)textShapeResult.error());
        return result;
    }

    const priv::TextShapeDataPtr_NEW textShapeData = textShapeResult.moveValue();
    if (textShapeData == nullptr) {
        return result;
    }

    const auto ConvertQuad = [](const priv::PlacedGlyph_pr::QuadCorners &qc)->PlacedGlyph::QuadCorners {
        const auto ConvertVec = [](const compat::Vector2f &vf)->Vector2d { return Vector2d { vf.x, vf.y }; };
        return PlacedGlyph::QuadCorners {
        ConvertVec(qc.topLeft),
        ConvertVec(qc.topRight),
        ConvertVec(qc.bottomLeft),
        ConvertVec(qc.bottomRight),
    }; };

    for (const priv::PlacedGlyph_pr &pg : textShapeData->placedGlyphs) {
        PlacedGlyph pgn;
        pgn.glyphCodepoint = pg.glyphCodepoint;
        pgn.quadCorners = ConvertQuad(pg.quadCorners);
        pgn.color = pg.color;
        pgn.fontFaceId = pg.fontFaceId;
        // TODO: Matus: This should not be here at all
        pgn.temp.format.size = pg.temp.size;
        pgn.temp.dimensions.ascender = pg.temp.ascender;
        result.placedGlyphs.emplace_back(pgn);
    }
    result.textTransform = convertMatrix(textShapeData->textTransform);
    result.unstretchedTextBounds = convertRect(textShapeData->unstretchedTextBounds);

    return result;
}

DrawTextResult drawText_NEW(ContextHandle ctx,
                            TextShapeHandle textShape,
                            void* pixels, int width, int height,
                            const DrawOptions& drawOptions) {
    return drawText_NEW_Inner(ctx, results_NEW[textShape], pixels, width, height, drawOptions);
}

DrawTextResult drawText_NEW_Inner(ContextHandle ctx,
                                  const ShapeTextResult_NEW &textShape_NEW,
                                  void *outputBuffer, int width, int height,
                                  const DrawOptions &drawOptions)
{
    if (ctx == nullptr) {
        return {};
    }

//    if (textShape && sanitizeShape(ctx, textShape)) {

    const compat::Rectangle viewArea = drawOptions.viewArea.has_value()
        ? convertRect(drawOptions.viewArea.value())
        : compat::INFINITE_BOUNDS;

    priv::PlacedGlyphs_pr pgs;
    const auto ConvertQuad = [](const PlacedGlyph::QuadCorners &qc)->priv::PlacedGlyph_pr::QuadCorners {
        const auto ConvertVec = [](const Vector2d &v)->compat::Vector2f { return compat::Vector2f { (float)v.x, (float)v.y }; };
        return priv::PlacedGlyph_pr::QuadCorners {
            ConvertVec(qc.topLeft),
            ConvertVec(qc.topRight),
            ConvertVec(qc.bottomLeft),
            ConvertVec(qc.bottomRight),
    }; };
    for (const PlacedGlyph &pg : textShape_NEW.placedGlyphs) {
        priv::PlacedGlyph_pr pgn;
        pgn.glyphCodepoint = pg.glyphCodepoint;
        pgn.quadCorners = ConvertQuad(pg.quadCorners);
        pgn.color = pg.color;
        pgn.fontFaceId = pg.fontFaceId;

        // TODO: Matus: This should not be here at all
        pgn.temp.size = pg.temp.format.size;
        pgn.temp.ascender = pg.temp.dimensions.ascender;

        pgs.emplace_back(pgn);
    }

    const compat::Matrix3f textTransform = convertMatrix(textShape_NEW.textTransform);

    const compat::FRectangle unstretchedTextBounds = convertRect(textShape_NEW.unstretchedTextBounds);
    const compat::FRectangle stretchedTextBounds = unstretchedTextBounds * drawOptions.scale;

    const priv::TextDrawResult result = priv::drawText_NEW(*ctx,
                                                           textTransform,
                                                           stretchedTextBounds,
                                                           outputBuffer, width, height,
                                                           drawOptions.scale,
                                                           viewArea,
                                                           pgs);

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
