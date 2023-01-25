
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


TextShapeHandle shapePlacedText(ContextHandle ctx,
                                const octopus::Text& text)
{
    if (ctx == nullptr) {
        return nullptr;
    }

    priv::TextShapeResult textShapeResult = priv::shapeText(*ctx, text);
    if (!textShapeResult) {
        ctx->getLogger().error("Text shaping failed with error: {}", (int)textShapeResult.error());
        return nullptr;
    }

    ctx->shapes.emplace_back(std::make_unique<TextShape>(textShapeResult.moveValue()));

    priv::PlacedTextResult placedShapeResult = priv::shapePlacedText(*ctx, text);
    if (!placedShapeResult) {
        ctx->getLogger().error("Text shaping failed with error: {}", (int)placedShapeResult.error());
        return nullptr;
    }

    // TODO: Matus: Remove this temporary static container
    tmp_placedData[ctx->shapes.back().get()] = placedShapeResult.moveValue();

    return ctx->shapes.back().get();
}

DrawTextResult drawPlacedText(ContextHandle ctx,
                              TextShapeHandle textShape,
                              void* pixels, int width, int height,
                              const DrawOptions& drawOptions) {
    if (ctx == nullptr) {
        return {};
    }

    if (tmp_placedData.find(textShape) != tmp_placedData.end() && tmp_placedData[textShape] != nullptr) {
        const priv::PlacedTextData &placedTextData = *tmp_placedData[textShape];

        const compat::Rectangle viewArea = drawOptions.viewArea.has_value()
            ? convertRect(drawOptions.viewArea.value())
            : compat::INFINITE_BOUNDS;

        const priv::TextDrawResult result = priv::drawPlacedText(*ctx,
                                                                 placedTextData,
                                                                 pixels, width, height,
                                                                 drawOptions.scale,
                                                                 viewArea);

        if (result) {
            const auto& drawOutput = result.value();
            return {
                utils::castRectangle(drawOutput.drawBounds), utils::castMatrix(drawOutput.transform),
                false
            };
        }
    }
    return DrawTextResult {};
}

} // namespace textify
