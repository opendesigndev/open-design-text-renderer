
#include <open-design-text-renderer/text-renderer-api.h>

#include "../compat/affine-transform.h"
#include "../compat/basic-types.h"

#include "../fonts/FontManager.h"

#include "../text-renderer/Config.h"
#include "../text-renderer/Context.h"
#include "../text-renderer/text-renderer.h"
#include "../text-renderer/TextShape.h"
#include "../text-renderer/types.h"

#include "../utils/utils.h"
#include "../utils/Log.h"
#include "../utils/fmt.h"

#include "../vendor/fmt/core.h"

#include <octopus/text.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace odtr {
namespace {

compat::Rectangle convertRect(const odtr::Rectangle& r) {
    return compat::Rectangle{r.l, r.t, r.w, r.h};
}
compat::FRectangle convertRect(const odtr::FRectangle& r) {
    return compat::FRectangle{r.l, r.t, r.w, r.h};
}

bool sanitizeShape(ContextHandle ctx,
                   TextShapeHandle textShape)
{
    if (textShape->dirty) {
        priv::PlacedTextResult placedShapeResult = priv::shapePlacedText(*ctx, *textShape->input);
        if (!placedShapeResult) {
            ctx->getLogger().error("Text reshaping failed with error: {}", errorToString(placedShapeResult.error()));
            return false;
        }

        textShape->data = placedShapeResult.moveValue();
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
    if (text.value.empty()) {
        return nullptr;
    }

    priv::TextShapeInputPtr textShapeInput = priv::preprocessText(*ctx, text);
    if (textShapeInput == nullptr) {
        ctx->getLogger().error("Text preprocessing failed.");
        return nullptr;
    }

    priv::PlacedTextResult placedShapeResult = priv::shapePlacedText(*ctx, *textShapeInput);
    if (!placedShapeResult) {
        ctx->getLogger().error("Text shaping failed with error: {}", errorToString(placedShapeResult.error()));
        return nullptr;
    }

    ctx->shapes.emplace_back(std::make_unique<TextShape>(std::move(textShapeInput), placedShapeResult.moveValue()));
    return ctx->shapes.back().get();
}

void destroyTextShapes(ContextHandle ctx,
                       TextShapeHandle* textShapes,
                       size_t count)
{
    if (ctx == nullptr) {
        return;
    }

    for (TextShapeHandle* textShape = textShapes; textShape != textShapes + count; ++textShape) {
        if (textShape != nullptr && *textShape != nullptr) {
            (*textShape)->deactivate();
        }
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

    priv::TextShapeInputPtr textShapeInput = priv::preprocessText(*ctx, text);
    if (textShapeInput == nullptr) {
        ctx->getLogger().error("Text preprocessign failed.");
        return false;
    }

    priv::PlacedTextResult textShapeResult = priv::shapePlacedText(*ctx, *textShapeInput);
    if (!textShapeResult) {
        ctx->getLogger().error("reshaping of a text failed with error: {}", (int)textShapeResult.error());
        return false;
    }

    textShape->input = std::move(textShapeInput);
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
        return utils::castFRectangle(convertRect(textShape->getData().textBounds));
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

    const compat::FRectangle textBounds = convertRect(textShape->getData().textBounds);
    return textShape && textBounds.contains(x, y);
}

Dimensions getDrawBufferDimensions(ContextHandle ctx,
                                   TextShapeHandle textShape,
                                   const DrawOptions& drawOptions)
{
    if (ctx == nullptr) {
        return {};
    }

    if (textShape && sanitizeShape(ctx, textShape)) {
        const compat::Rectangle viewArea = drawOptions.viewArea.has_value() ? convertRect(drawOptions.viewArea.value()) : compat::INFINITE_BOUNDS;
        const compat::Rectangle drawBounds = priv::computeDrawBounds(*ctx, textShape->getData(), drawOptions.scale, viewArea);

        return { drawBounds.w, drawBounds.h };
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
        const compat::Rectangle viewArea = drawOptions.viewArea.has_value()
            ? convertRect(drawOptions.viewArea.value())
            : compat::INFINITE_BOUNDS;

        const priv::TextDrawResult result = priv::drawPlacedText(*ctx,
                                                                 textShape->getData(),
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

const PlacedTextData *getShapedText(ContextHandle ctx,
                                    TextShapeHandle textShape) {
    if (ctx == nullptr) {
        return nullptr;
    }

    if (textShape && sanitizeShape(ctx, textShape) && textShape->data != nullptr) {
        return textShape->data.get();
    }
    return nullptr;
}

} // namespace odtr
