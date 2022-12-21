#pragma once

#include "errors.h"
#include "FormattedText.h"
#include "ParagraphShape.h"
#include "text-format.h"

#include "common/result.hpp"
#include "compat/basic-types.h"

#include <memory>

// Forward declarations
namespace octopus {
    struct Text;
}
namespace textify {
    struct Context;
}

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
using TextShapeResult = Result<TextShapeDataPtr, TextShapeError>;

struct TextDrawOutput
{
    compat::Rectangle drawBounds;
    compat::Matrix3f transform;
};

enum class TextDrawError
{
    OK,
    INVALID_SCALE,
    PARAGRAPHS_TYPESETING_ERROR,
    DRAW_BOUNDS_ERROR
};

using TextDrawResult = Result<TextDrawOutput, TextDrawError>;

// TODO: Matus: Params from the formatted text. Move or cleanup?
struct FormattedTextParams {
    VerticalAlign verticalAlign;
    BoundsMode boundsMode;
    BaselinePolicy baselinePolicy;
    OverflowPolicy overflowPolicy;
};

/// List all font face names that have not been loaded to the context's FontManager.
FacesNames listMissingFonts(Context &ctx,
                            const octopus::Text& text);

TextShapeResult shapeText(Context &ctx,
                          const octopus::Text& text);

TextShapeResult reshapeText(Context &ctx,
                            TextShapeDataPtr&& textShapeData);

TextShapeResult shapeTextInner(Context &ctx,
                FormattedTextPtr text,
                const FrameSizeOpt &frameSize,
                const compat::Matrix3f &textTransform);

TextDrawResult drawText(Context &ctx,
                        const TextShapeData& shapeData,
                        void* pixels,
                        int width,
                        int height,
                        float scale,
                        bool dry,
                        const compat::Rectangle& viewArea);

// TODO: Matus: Cleanup this function - make sure all the arguments are optimal etc.
TextDrawResult drawText(Context &ctx,
                        const ParagraphShapes &paragraphShapes,
                        const compat::Matrix3f &textTransform,
                        const compat::FRectangle &unscaledTextBounds,
                        const FormattedTextParams &textParams,
                        float baseline,
                        void *pixels,
                        int width,
                        int height,
                        float scale,
                        bool dry,
                        const compat::Rectangle &viewArea);

TextDrawResult drawTextInner(Context &ctx,
                             bool dry, // Only compute the boundaries, the actual drawing does not take place.
                             const FormattedTextParams &textParams,
                             const compat::FRectangle& unscaledTextBounds,
                             float baseline,
                             RenderScale scale,
                             const compat::FRectangle& viewArea,
                             const ParagraphShapes& paragraphShapes,
                             Pixel32* pixels,
                             int width,
                             int height);

/// Draw individual ParagraphShapes.
ParagraphShape::DrawResults drawParagraphsInner(Context &ctx,
                                                const ParagraphShapes &shapes,
                                                BaselinePolicy baselinePolicy,
                                                OverflowPolicy overflowPolicy,
                                                int textWidth,
                                                RenderScale scale,
                                                VerticalPositioning &positioning,
                                                float &caretVerticalPos);

} // namespace priv
} // namespace textify
