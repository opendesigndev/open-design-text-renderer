
#pragma once

#include <memory>

#include "FormattedText.h"
#include "ParagraphShape.h"

namespace textify {
namespace priv {

using UsedFaces = std::unordered_set<std::string>;
using FrameSizeOpt = std::optional<compat::Vector2f>;

struct TextShapeData
{
    TextShapeData(FormattedTextPtr text,
                  FrameSizeOpt frameSize,
                  const compat::Matrix3f& textTransform,
                  ParagraphShapes&& shapes,
                  const compat::FRectangle& boundsNoTransform,
                  const compat::FRectangle& boundsTransformed,
                  float baseline);

    // input properties
    FormattedTextPtr formattedText;
    FrameSizeOpt frameSize;
    compat::Matrix3f textTransform;
    UsedFaces usedFaces;

    // processed
    ParagraphShapes paragraphShapes;
    compat::FRectangle textBoundsNoTransform;
    compat::FRectangle textBoundsTransformed;
    float baseline;
};
using TextShapeDataPtr = std::unique_ptr<TextShapeData>;

} // namespace priv
} // namespace textify
