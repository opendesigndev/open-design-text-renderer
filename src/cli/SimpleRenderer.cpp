
#include "cli/SimpleRenderer.h"
#include "cli/Canvas.h"
#include "cli/octopus-utils.h"
#include "compat/basic-types.h"
#include "fonts/FontManager.h"
#include "octopus/fill.h"
#include "octopus/layer.h"
#include "octopus/shape.h"
#include "textify/textify-api.h"

#include "utils/Log.h"
#include "utils/utils.h"
#include "utils/fmt.h"

#include "vendor/fmt/color.h"
#include "vendor/fmt/core.h"

#include <octopus/fill.h>
#include <octopus/layer.h>
#include <octopus/octopus.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>

namespace textify {
namespace cli {

namespace unsafe {
namespace {
template <typename T, typename U>
T cast(const U& x)
{
    return *reinterpret_cast<const T*>(&x);
}
}
}

namespace {

void errorFunc(const std::string& msg) {
    fmt::print(fg(fmt::color::white) | bg(fmt::color::crimson) | fmt::emphasis::bold, "[ERROR] {}", msg);
    fmt::print("\n");
}

void warnFunc(const std::string& msg) {
    fmt::print("[WARN] {}\n", msg);
}

void infoFunc(const std::string& msg) {
    fmt::print("[INFO] {}\n", msg);
}

}

SimpleRenderer::SimpleRenderer(const octopus::Octopus& oct, float scale)
    : octopus_(oct),
      textifyCtx_(textify::createContext({})),
      log(std::make_unique<utils::Log>(errorFunc, warnFunc, infoFunc)),
      scale_(scale)
{
    // preload emoji font
    // textify::addFontFile(textifyCtx_, textify::FontManager::DEFAULT_EMOJI_FONT, "", "fonts/NotoColorEmoji.ttf", false);
    if (!textify::addFontFile(textifyCtx_, textify::FontManager::DEFAULT_EMOJI_FONT, "", "fonts/AppleColorEmoji.ttc", false)) {
        log->info("Emoji font not preloaded");
    }
}

SimpleRenderer::~SimpleRenderer()
{
    textify::destroyContext(textifyCtx_);
}

bool SimpleRenderer::renderToFile(const std::string& filename)
{
    auto w = octopus_.dimensions.has_value() ? static_cast<int>(std::ceil(octopus_.dimensions->width)) : 256;
    auto h = octopus_.dimensions.has_value() ? static_cast<int>(std::ceil(octopus_.dimensions->height)) : 256;

    auto canvas = Canvas{scale(w), scale(h)};

    if (!octopus_.content.has_value()) {
        log->error("no content");
        return false;
    }

    auto result = renderLayer(*octopus_.content.get(), canvas, compat::Matrix3f::identity);
    if (!result) {
        log->info("there are some layer that has not been rendered");
    }

    return canvas.save(filename);
}

bool SimpleRenderer::renderLayer(const octopus::Layer& layer, Canvas& canvas, const compat::Matrix3f& transform)
{
    auto lt = convertMatrix(layer.transform);
    lt.m[2][0] *= scale_;
    lt.m[2][1] *= scale_;
    auto T = transform * lt;

    switch (layer.type) {
        case octopus::Layer::Type::GROUP:
        case octopus::Layer::Type::MASK_GROUP:
            if (layer.layers.has_value()) {
                for (const auto& child : layer.layers.value()) {
                    renderLayer(child, canvas, T);
                }
            }
            break;
        case octopus::Layer::Type::TEXT:
            renderTextLayer(layer, canvas, T);
            break;
        case octopus::Layer::Type::SHAPE:
            renderShapeLayer(layer, canvas, T);
            break;
        default:
            break;
    }
    return true;
}

bool SimpleRenderer::renderTextLayer(const octopus::Layer& layer, Canvas& canvas, const compat::Matrix3f& transform)
{
    if (layer.type != octopus::Layer::Type::TEXT ||  !layer.text.has_value()) {
        return false;
    }


    auto viewArea = std::optional<textify::Rectangle>{};

    // to test view area
    // auto viewArea = textify::Rectangle{56, 76, 163, 117};
    // auto viewArea = textify::Rectangle{109, 138, 163, 117};
    // auto viewArea = textify::Rectangle{60, 79, 163, 117};

    if (viewArea_.has_value()) {
        // auto viewRectLayer = utils::transform(utils::toFRectangle(viewArea_.value()), convertMatrix(layer.transform));
        // viewArea = unsafe::cast<textify::Rectangle>(utils::outerRect(viewRectLayer));
        // viewArea = unsafe::cast<textify::Rectangle>(viewArea_.value());
    }

    return renderText(layer.text.value(), canvas, transform, viewArea);
}

bool SimpleRenderer::renderText(const octopus::Text& text, Canvas& canvas, const compat::Matrix3f& transform,
        const std::optional<textify::Rectangle>& viewArea)
{
    for (const auto& missingFont : textify::listMissingFonts(textifyCtx_, text)) {

        if (loadFont(missingFont)) {
            log->info("loaded missing font: {}", missingFont);
        } else {
            log->error("failed to load missing font: {}", missingFont);
            return false;
        }
    }

    auto textShape = textify::shapeText(textifyCtx_, text);

    auto bounds = textify::getBounds(textifyCtx_, textShape);
    utils::printRect("text bounds:", bounds);

    if (viewArea.has_value()) {
        const auto& r = viewArea.value();
        log->info("drawing text after reading cutout: {}, {} | {} x {}", r.l, r.t, r.w, r.h);
    }
    textify::DrawOptions drawOptions = {
        scale_,
        viewArea
    };

    auto [width, height] = textify::getDrawBufferDimensions(textifyCtx_, textShape, drawOptions);

    auto bitmap = compat::BitmapRGBA(width, height);
    bitmap.clear();
    // bitmap.fill(0x1fcc11cc);

    auto drawResult = textify::drawText(textifyCtx_, textShape, bitmap.pixels(), width, height, drawOptions);
    if (drawResult.error) {
        log->error("failed to shape text content: {}...", text.value.substr(0, std::min(text.value.size(), (size_t) 16)));
        return false;
    }

    auto canvasTransform = transform * unsafe::cast<textify::compat::Matrix3f>(drawResult.transform);

    canvas.blendBitmap(
            bitmap,
            unsafe::cast<textify::compat::Rectangle>(drawResult.bounds),
            canvasTransform
        );

    textify::destroyTextShapes(textifyCtx_, &textShape, 1);

    return true;
}

bool SimpleRenderer::renderShapeLayer(const octopus::Layer& layer, Canvas& canvas, const compat::Matrix3f& transform)
{
    if (layer.type != octopus::Layer::Type::SHAPE || !layer.shape.has_value()) {
        return false;
    }

    auto res = renderShape(layer.shape.value(), canvas, transform);
    if (res && layer.name == "cutout") {
        /*
        log->info("cutout layer: {} x {}", res.value().w, res.value().h);
        viewArea_ = res.moveValue();
        */
        // viewArea_ = res.moveValue();

        auto viewRectLayer = utils::transform(utils::toFRectangle(res.value()), convertMatrix(layer.transform));
        viewArea_ = utils::outerRect(viewRectLayer);
    }
    return res;
}

SimpleRenderer::RenderShapeResult SimpleRenderer::renderShape(const octopus::Shape& shape, Canvas& canvas, const compat::Matrix3f& transform) const
{
    // knows how to render a solo rectangle only
    if (!shape.path.has_value()) {
        return false;
    }

    const auto& path = shape.path.value();

    if (path.type != octopus::Path::Type::RECTANGLE || !path.rectangle.has_value()) {
        return false;
    }

    if (shape.fills.empty() || shape.fills.front().type != octopus::Fill::Type::COLOR || !shape.fills.front().color.has_value()) {
        return false;
    }

    if (!shape.fills.front().visible) {
        return true;
    }

    auto color = convertColor(shape.fills.front().color.value());

    const auto& rect = path.rectangle.value();

    auto l = scale(static_cast<int>(rect.x0));
    auto t = scale(static_cast<int>(rect.y0));
    auto w = scale(static_cast<int>(std::round(rect.x1 - rect.x0)));
    auto h = scale(static_cast<int>(std::round(rect.y1 - rect.y0)));

    auto rectBounds = compat::Rectangle{l, t, w, h};

    Canvas tmp(w, h);
    tmp.fill(color);

    auto rectTransform = transform * convertMatrix(path.transform);

    canvas.blendCanvas(tmp, rectBounds, rectTransform);

    return rectBounds;
}

bool SimpleRenderer::loadFont(const std::string& name)
{
    auto tryLoad = [&](const std::string& name, const std::string& ext) {
        return textify::addFontFile(textifyCtx_, name, "", fmt::format("fonts/{}.{}", name, ext), false);
    };

    for (const auto& ext : {"ttf", "otf", "ttc"}) {
        if (tryLoad(name, ext)) {
            return true;
        }
    }

    return false;
}

int SimpleRenderer::scale(int x) const {
    return (int)std::ceil(x * scale_);
}

}
}
