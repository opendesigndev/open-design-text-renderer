#pragma once

#include "fonts/FontManager.h"
// #include "textify.h"
#include "textify/Config.h"
#include "TextShape.h"
#include "utils/Log.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace textify {

class FontManager;

struct Context
{
    priv::Config config;

    std::unique_ptr<utils::Log> logger = utils::Log::createNoLog();
    std::unique_ptr<FontManager> fontManager;

    std::vector<std::unique_ptr<TextShape>> shapes;

    const utils::Log& getLogger() const;

    const FontManager& getFontManager() const;
    FontManager& getFontManager();

    int removeInactiveShapes();
};

}
