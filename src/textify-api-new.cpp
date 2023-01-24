
#include "textify/textify-api-new.h"

#include <map>

#include "textify/textify.h"
#include "utils/utils.h"


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
}

PlacedGlyph convertToPlacedGlyph(const priv::GlyphShape &glyph) {
    PlacedGlyph result;

    result.glyphCodepoint = glyph.codepoint;
    result.quadCorners = {};
    result.color = glyph.format.color;
    result.fontSize = glyph.format.size;
    result.fontFaceId = glyph.format.faceId;

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
        const auto ConvertVec = [](const compat::Vector2f &vf)->Vector2f { return Vector2f { vf.x, vf.y }; };
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
        pgn.fontSize = pg.fontSize;
        pgn.fontFaceId = pg.fontFaceId;

        result.placedGlyphs.emplace_back(pgn);
    }

    for (const priv::PlacedDecoration_pr &pd : textShapeData->placedDecorations) {
        PlacedDecoration pdn;

        pdn.type = static_cast<PlacedDecoration::Type>(pd.type);
        pdn.color = pd.color;
        pdn.thickness = pd.thickness;
        pdn.yOffset = pd.yOffset;
        pdn.xRange.first = pd.xRange.first;
        pdn.xRange.last = pd.xRange.last;

        result.placedDecorations.emplace_back(pdn);
    }

    result.textBounds = convertRect(textShapeData->unstretchedTextBounds);

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

    const compat::Rectangle viewArea = drawOptions.viewArea.has_value()
        ? convertRect(drawOptions.viewArea.value())
        : compat::INFINITE_BOUNDS;

    priv::PlacedGlyphs_pr pgs;
    const auto ConvertQuad = [](const PlacedGlyph::QuadCorners &qc)->priv::PlacedGlyph_pr::QuadCorners {
        const auto ConvertVec = [](const Vector2f &v)->compat::Vector2f { return compat::Vector2f { (float)v.x, (float)v.y }; };
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
        pgn.fontSize = pg.fontSize;
        pgn.fontFaceId = pg.fontFaceId;

        pgs.emplace_back(pgn);
    }

    priv::PlacedDecorations_pr pds;
    for (const PlacedDecoration &pd : textShape_NEW.placedDecorations) {
        priv::PlacedDecoration_pr pdn;

        pdn.type = static_cast<priv::PlacedDecoration_pr::Type>(pd.type);
        pdn.color = pd.color;
        pdn.thickness = pd.thickness;
        pdn.yOffset = pd.yOffset;
        pdn.xRange.first = pd.xRange.first;
        pdn.xRange.last = pd.xRange.last;

        pds.emplace_back(pdn);
    }

    const compat::FRectangle textBounds = convertRect(textShape_NEW.textBounds);
    const compat::FRectangle stretchedTextBounds = textBounds * drawOptions.scale;

    const priv::TextDrawResult result = priv::drawText_NEW(*ctx,
                                                           stretchedTextBounds,
                                                           outputBuffer, width, height,
                                                           drawOptions.scale,
                                                           viewArea,
                                                           pgs, pds);

    if (result) {
        const auto& drawOutput = result.value();
        return {
            utils::castRectangle(drawOutput.drawBounds), utils::castMatrix(drawOutput.transform),
            false
        };
    }

    return DrawTextResult {};
}

} // namespace textify
