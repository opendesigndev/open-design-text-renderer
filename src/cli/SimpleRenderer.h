#pragma once

#include "cli/Canvas.h"

#include "common/result.hpp"
#include "compat/basic-types.h"
#include "octopus/shape.h"
#include "octopus/text.h"
#include "textify/textify-api.h"
#include "utils/Log.h"
#include <cstddef>

namespace octopus {
    struct Octopus;
    struct Layer;
}

namespace textify {
namespace cli {

class SimpleRenderer
{
public:
    SimpleRenderer(const octopus::Octopus& oct, float scale);
    ~SimpleRenderer();

    bool renderToFile(const std::string& filename);

private:
    bool renderLayer(const octopus::Layer& layer, Canvas& canvas, const compat::Matrix3f& transform);

    bool renderTextLayer(const octopus::Layer& layer, Canvas& canvas, const compat::Matrix3f& transform);
    bool renderText(const octopus::Text& text, Canvas& canvas, const compat::Matrix3f& transform, const std::optional<textify::Rectangle>& viewArea);

    bool renderShapeLayer(const octopus::Layer& layer, Canvas& canvas, const compat::Matrix3f& transform);

    using RenderShapeResult = Result<compat::Rectangle, bool>;
    RenderShapeResult renderShape(const octopus::Shape& shape, Canvas& canvas, const compat::Matrix3f& transform) const;

    bool loadFont(const std::string& name);
    int scale(int x) const;

    const octopus::Octopus& octopus_;
    textify::ContextHandle textifyCtx_;
    std::unique_ptr<utils::Log> log;
    float scale_;
    std::optional<compat::Rectangle> viewArea_;
};

}
}
