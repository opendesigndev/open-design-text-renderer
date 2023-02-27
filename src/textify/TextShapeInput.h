
#pragma once

#include <memory>

#include "FormattedText.h"

namespace textify {
namespace priv {

using UsedFaces = std::unordered_set<std::string>;
using FrameSizeOpt = std::optional<compat::Vector2f>;

struct TextShapeInput
{
    TextShapeInput(FormattedTextPtr text,
                   const FrameSizeOpt &frameSize,
                   const compat::Matrix3f& textTransform);

    // input properties
    FormattedTextPtr formattedText;
    FrameSizeOpt frameSize;
    compat::Matrix3f textTransform;
    UsedFaces usedFaces;
};
using TextShapeInputPtr = std::unique_ptr<TextShapeInput>;

} // namespace priv
} // namespace textify
