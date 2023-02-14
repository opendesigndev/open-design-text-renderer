
#include <textify/textify-api.h>

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

#include <octopus/text.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace textify {
namespace {

compat::Rectangle convertRect(const textify::Rectangle& r) {
    return compat::Rectangle{r.l, r.t, r.w, r.h};
}
compat::FRectangle convertRect(const textify::FRectangle& r) {
    return compat::FRectangle{r.l, r.t, r.w, r.h};
}

bool sanitizeShape(ContextHandle ctx,
                   TextShapeHandle textShape)
{
    if (textShape->dirty) {
        priv::TextShapeResult textShapeResult = priv::reshapeText(*ctx, std::move(textShape->data));

        if (!textShapeResult) {
            return false;
        }
        textShape->data = textShapeResult.moveValue();
        textShape->dirty = false;
    }
    return true;
}

}

ContextHandle createContext(const ContextOptions& options)
{
    auto logger = std::make_unique<utils::Log>(options.errorFunc, options.warnFunc, options.infoFunc);
    auto fontManager = std::make_unique<FontManager>(*logger.get());

    return new Context{
        priv::Config{},
        std::move(logger),
        std::move(fontManager),
    };
}

void destroyContext(ContextHandle ctx)
{
    if (ctx) {
        delete ctx;
    }
}

bool addFontFile(ContextHandle ctx,
                 const std::string& postScriptName,
                 const std::string& inFontFaceName,
                 const std::string& filename,
                 bool overwrite)
{
    if (ctx == nullptr) {
        return false;
    }

    auto facesToUpdate = overwrite ? ctx->fontManager->listFacesInStorage(filename) : FacesNames{};
    if (ctx->fontManager->faceExists(postScriptName)) {
        facesToUpdate.push_back(postScriptName);
    }

    auto result = ctx->fontManager->loadFaceFromFileAs(filename, postScriptName, inFontFaceName);

    for (const auto& shape : ctx->shapes) {
        for (const auto& face : facesToUpdate) {
            shape->onFontFaceChanged(face);
        }
    }

    return result;
}

bool addFontBytes(ContextHandle ctx,
                  const std::string& postScriptName,
                  const std::string& inFontFaceName,
                  const std::uint8_t* data,
                  size_t length,
                  bool overwrite)
{
    if (ctx == nullptr) {
        return false;
    }

    auto facesToUpdate = FacesNames{}; // TODO overwrite?
    if (ctx->fontManager->faceExists(postScriptName)) {
        facesToUpdate.push_back(postScriptName);
    }

    // TODO get rid of const_cast
    auto result = ctx->fontManager->loadFaceAs(postScriptName, postScriptName, inFontFaceName, BufferView(const_cast<std::uint8_t *>(data), length));

    for (const auto& shape : ctx->shapes) {
        for (const auto& face : facesToUpdate) {
            shape->onFontFaceChanged(face);
        }
    }

    return result;
}

std::vector<std::string> listMissingFonts(ContextHandle ctx,
                                          const octopus::Text& text)
{
    if (ctx == nullptr) {
        return {};
    }
    return priv::listMissingFonts(*ctx, text);
}

TextShapeHandle shapeText(ContextHandle ctx,
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

    return ctx->shapes.back().get();
}

void destroyTextShapes(ContextHandle ctx,
                       TextShapeHandle* textShapes,
                       size_t count)
{
    if (ctx == nullptr) {
        return;
    }

    for (auto textShape = textShapes; textShape != textShapes + count; ++textShape) {
        (*textShape)->deactivate();
    }

    ctx->removeInactiveShapes();
}

bool reshapeText(ContextHandle ctx,
                 TextShapeHandle textShape,
                 const octopus::Text& text)
{
    if (ctx == nullptr) {
        return false;
    }

    priv::TextShapeResult textShapeResult = priv::shapeText(*ctx, text);
    if (!textShapeResult) {
        ctx->getLogger().error("reshaping of a text failed with error: {}", (int)textShapeResult.error());
        return false;
    }

    textShape->data = textShapeResult.moveValue();
    return true;
}

FRectangle getBounds(ContextHandle ctx,
                     TextShapeHandle textShape)
{
    if (ctx == nullptr) {
        return {};
    }

    if (textShape && sanitizeShape(ctx, textShape)) {
        return utils::castFRectangle(textShape->isPlaced()
                                     ? convertRect(textShape->getPlacedData().textBounds)
                                     : textShape->getData().textBoundsTransformed);
    }
    return {};
}

bool intersect(ContextHandle ctx,
               TextShapeHandle textShape,
               float x,
               float y,
               float radius)
{
    if (ctx == nullptr) {
        return false;
    }

    return textShape && textShape->getData().textBoundsTransformed.contains(x, y);
}

Dimensions getDrawBufferDimensions(ContextHandle ctx,
                                   TextShapeHandle textShape,
                                   const DrawOptions& drawOptions)
{
    if (ctx == nullptr) {
        return {};
    }

    if (textShape && sanitizeShape(ctx, textShape)) {
        if (textShape->isPlaced()) {
            const compat::Rectangle viewArea = drawOptions.viewArea.has_value() ? convertRect(drawOptions.viewArea.value()) : compat::INFINITE_BOUNDS;
            const compat::Rectangle drawBounds = priv::computeDrawBounds(*ctx, textShape->getPlacedData(), drawOptions.scale, viewArea);

            return { drawBounds.w, drawBounds.h };
        } else {
            const compat::Rectangle viewArea = drawOptions.viewArea.has_value() ? convertRect(drawOptions.viewArea.value()) : compat::INFINITE_BOUNDS;
            const priv::TextDrawResult result = priv::drawText(*ctx, textShape->getData(), drawOptions.scale, viewArea, nullptr, 0, 0, true);

            if (result) {
                const int w = result.value().drawBounds.w;
                const int h = result.value().drawBounds.h;

                return {w, h};
            }
        }
    }
    return {};
}

DrawTextResult drawText(ContextHandle ctx,
                        TextShapeHandle textShape,
                        void* pixels, int width, int height,
                        const DrawOptions& drawOptions)
{
    if (ctx == nullptr) {
        return {{}, {}, true};
    }

    if (textShape && sanitizeShape(ctx, textShape)) {
        const compat::Rectangle viewArea = drawOptions.viewArea.has_value() ? convertRect(drawOptions.viewArea.value()) : compat::INFINITE_BOUNDS;
        const priv::TextDrawResult result = priv::drawText(*ctx, textShape->getData(), drawOptions.scale, viewArea, pixels, width, height, false);

        if (result) {
            const auto& drawOutput = result.value();
            return {utils::castRectangle(drawOutput.drawBounds), utils::castMatrix(drawOutput.transform), false};
        }
    }

    return {{}, {}, true};
}

TextShapeHandle shapePlacedText(ContextHandle ctx,
                                const octopus::Text& text)
{
    if (ctx == nullptr) {
        return nullptr;
    }

    // TODO: Matus: Return just placed text?
    priv::TextShapeResult textShapeResult = priv::shapeText(*ctx, text);
    priv::PlacedTextResult placedShapeResult = priv::shapePlacedText(*ctx, text);

    if (!textShapeResult) {
        ctx->getLogger().error("Text shaping failed with error: {}", (int)textShapeResult.error());
    }
    if (!placedShapeResult) {
        ctx->getLogger().error("Text shaping failed with error: {}", (int)placedShapeResult.error());
    }

    if (!textShapeResult && !placedShapeResult) {
        return nullptr;
    } else if (!textShapeResult) {
        ctx->shapes.emplace_back(std::make_unique<TextShape>(placedShapeResult.moveValue()));
    } else if (!placedShapeResult) {
        ctx->shapes.emplace_back(std::make_unique<TextShape>(textShapeResult.moveValue()));
    } else {
        ctx->shapes.emplace_back(std::make_unique<TextShape>(textShapeResult.moveValue(), placedShapeResult.moveValue()));
    }

    return ctx->shapes.back().get();
}

DrawTextResult drawPlacedText(ContextHandle ctx,
                              TextShapeHandle textShape,
                              void* pixels, int width, int height,
                              const DrawOptions& drawOptions) {
    if (ctx == nullptr) {
        return {{}, {}, true};
    }

    if (textShape && sanitizeShape(ctx, textShape)) {
        const PlacedTextData &placedTextData = textShape->getPlacedData();

        const compat::Rectangle viewArea = drawOptions.viewArea.has_value()
            ? convertRect(drawOptions.viewArea.value())
            : compat::INFINITE_BOUNDS;

        const priv::TextDrawResult result = priv::drawPlacedText(*ctx,
                                                                 placedTextData,
                                                                 drawOptions.scale,
                                                                 viewArea,
                                                                 pixels, width, height);

        if (result) {
            const auto& drawOutput = result.value();
            return {
                utils::castRectangle(drawOutput.drawBounds), utils::castMatrix(drawOutput.transform),
                false
            };
        }
    }

    return {{}, {}, true};
}

} // namespace textify
