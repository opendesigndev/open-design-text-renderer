
#include <textify/textify-api-new.h>

#include <map>

#include "textify/PlacedTextData.h"
#include "textify/textify.h"
#include "utils/utils.h"


namespace textify {
static std::map<TextShapeHandle, priv::PlacedTextDataPtr> tmp_placedData;
}

namespace textify {
namespace {
compat::Rectangle convertRect(const textify::Rectangle &r) {
    return compat::Rectangle{r.l, r.t, r.w, r.h};
}
}


priv::PlacedTextDataPtr shapeText_NEW_Inner(ContextHandle ctx,
                                            const octopus::Text &text) {
    if (ctx == nullptr) {
        return nullptr;
    }

    priv::TextShapeResult_NEW textShapeResult = priv::shapeText_NEW(*ctx, text);
    if (!textShapeResult) {
        ctx->getLogger().error("shaping of a text failed with error: {}", (int)textShapeResult.error());
        return nullptr;
    }

    const priv::TextShapeDataPtr_NEW textShapeData = textShapeResult.moveValue();
    if (textShapeData == nullptr) {
        return nullptr;
    }

    return std::make_unique<priv::PlacedTextData>(textShapeData->placedGlyphs, textShapeData->placedDecorations, textShapeData->unstretchedTextBounds);
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

    tmp_placedData[ctx->shapes.back().get()] = shapeText_NEW_Inner(ctx, text);

    return ctx->shapes.back().get();
}

DrawTextResult drawText_NEW_Inner(ContextHandle ctx,
                                  const priv::PlacedTextData &placedTextData,
                                  void *outputBuffer, int width, int height,
                                  const DrawOptions &drawOptions)
{
    if (ctx == nullptr) {
        return {};
    }

    const compat::Rectangle viewArea = drawOptions.viewArea.has_value()
        ? convertRect(drawOptions.viewArea.value())
        : compat::INFINITE_BOUNDS;

    const compat::FRectangle stretchedTextBounds = placedTextData.textBounds * drawOptions.scale;

    const priv::TextDrawResult result = priv::drawText_NEW(*ctx,
                                                           stretchedTextBounds,
                                                           outputBuffer, width, height,
                                                           drawOptions.scale,
                                                           viewArea,
                                                           placedTextData.glyphs, placedTextData.decorations);

    if (result) {
        const auto& drawOutput = result.value();
        return {
            utils::castRectangle(drawOutput.drawBounds), utils::castMatrix(drawOutput.transform),
            false
        };
    }

    return DrawTextResult {};
}

DrawTextResult drawText_NEW(ContextHandle ctx,
                            TextShapeHandle textShape,
                            void* pixels, int width, int height,
                            const DrawOptions& drawOptions) {
    if (tmp_placedData.find(textShape) != tmp_placedData.end() && tmp_placedData[textShape] != nullptr) {
        return drawText_NEW_Inner(ctx, *tmp_placedData[textShape], pixels, width, height, drawOptions);
    }
    return DrawTextResult {};
}

} // namespace textify
