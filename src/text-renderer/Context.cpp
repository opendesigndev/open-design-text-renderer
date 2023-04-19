#include "Context.h"
#include "TextShape.h"
#include "text-renderer.h"
#include "../fonts/FontManager.h"

namespace odtr {

const utils::Log& Context::getLogger() const
{
    return *logger.get();
}

const FontManager& Context::getFontManager() const
{
    return *fontManager.get();
}

FontManager& Context::getFontManager()
{
    return *fontManager.get();
}

int Context::removeInactiveShapes()
{
    auto before = shapes.size();
    shapes.erase(
        std::remove_if(std::begin(shapes), std::end(shapes), [](const auto& shape) { return !shape->active; }),
        std::end(shapes)
    );
    return int(before - shapes.size());
}

}
