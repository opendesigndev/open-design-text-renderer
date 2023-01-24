
#include <textify/textify-api-new.h>

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

    result.placedGlyphs = textShapeData->placedGlyphs;
    result.placedDecorations = textShapeData->placedDecorations;

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

    const compat::FRectangle textBounds = convertRect(textShape_NEW.textBounds);
    const compat::FRectangle stretchedTextBounds = textBounds * drawOptions.scale;

    const priv::TextDrawResult result = priv::drawText_NEW(*ctx,
                                                           stretchedTextBounds,
                                                           outputBuffer, width, height,
                                                           drawOptions.scale,
                                                           viewArea,
                                                           textShape_NEW.placedGlyphs, textShape_NEW.placedDecorations);

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
