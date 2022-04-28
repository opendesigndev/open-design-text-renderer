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

bool sanitizeShape(Context* ctx, TextShapeHandle textShape)
{
    if (textShape->dirty) {
        auto textShapeResult = priv::reshapeText(ctx, std::move(textShape->data));

        if (!textShapeResult) {
            return false;
        }
        textShape->data = textShapeResult.moveValue();
        textShape->dirty = false;
    }
    return true;
}

}

Context* createContext(const ContextOptions& options)
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
    delete ctx;
}

bool addFontFile(Context* ctx, const std::string& postScriptName, const std::string& inFontFaceName, const std::string& filename, bool override)
{
    auto facesToUpdate = override ? ctx->fontManager->listFacesInStorage(filename) : FacesNames{};
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

bool addFontBytes(ContextHandle ctx, const std::string& postScriptName, const std::string& inFontFaceName, const std::uint8_t* data, size_t length, bool override)
{
    // TODO:
    return false;
}

std::vector<std::string> listMissingFonts(Context* ctx, const octopus::Text& text)
{
    return priv::listMissingFonts(ctx, text);
}

TextShapeHandle shapeText(Context* ctx, const octopus::Text& text)
{
    auto textShapeResult = priv::shapeText(ctx, text);
    if (!textShapeResult) {
        ctx->getLogger().error("shaping of a text failed with error: {}", (int)textShapeResult.error());
        return nullptr;
    }

    ctx->shapes.emplace_back(std::make_unique<TextShape>(textShapeResult.moveValue()));
    return ctx->shapes.back().get();
}

void destroyTextShapes(ContextHandle ctx, TextShapeHandle* textShapes, size_t count)
{
    for (auto textShape = textShapes; textShape != textShapes + count; ++textShape) {
        (*textShape)->deactivate();
    }

    ctx->removeInactiveShapes();
}

bool reshapeText(Context* ctx, TextShapeHandle textShape, const octopus::Text& text)
{
    auto textShapeResult = priv::shapeText(ctx, text);
    if (!textShapeResult) {
        ctx->getLogger().error("reshaping of a text failed with error: {}", (int)textShapeResult.error());
        return false;
    }

    textShape->data = textShapeResult.moveValue();
    return true;
}

FRectangle getBounds(Context* ctx, TextShapeHandle textShape)
{
    if (textShape && sanitizeShape(ctx, textShape)) {
        return utils::castFRectangle(textShape->getData().textBoundsTransformed);
    }
    return {};
}

bool intersect(ContextHandle ctx, TextShapeHandle textShape, float x, float y, float radius)
{
    return textShape && textShape->getData().textBoundsTransformed.contains(x, y);
}

Dimensions getDrawBufferDimensions(ContextHandle ctx, TextShapeHandle textShape, const DrawOptions& drawOptions)
{
    if (textShape && sanitizeShape(ctx, textShape)) {
        auto viewArea = drawOptions.viewArea.has_value() ? convertRect(drawOptions.viewArea.value()) : compat::INFINITE_BOUNDS;
        auto result = priv::drawText(ctx, textShape->getData(), nullptr, 0, 0, drawOptions.scale, true, viewArea);
        if (result) {
            int w = result.value().drawBounds.w;
            int h = result.value().drawBounds.h;

            return {w, h};
        }
    }
    return {};
}

DrawTextResult drawText(ContextHandle ctx, TextShapeHandle textShape, void* pixels, int width, int height, const DrawOptions& drawOptions)
{
    if (textShape && sanitizeShape(ctx, textShape)) {
        auto viewArea = drawOptions.viewArea.has_value() ? convertRect(drawOptions.viewArea.value()) : compat::INFINITE_BOUNDS;
        auto result = priv::drawText(ctx, textShape->getData(), pixels, width, height, drawOptions.scale, false, viewArea);
        if (result) {
            const auto& drawOutput = result.value();

            return {utils::castRectangle(drawOutput.drawBounds), utils::castMatrix(drawOutput.transform), false};
        }
    }
    return {{}, {}, true};
}

} // namespace textify
