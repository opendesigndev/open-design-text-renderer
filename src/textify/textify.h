#pragma once

#include "errors.h"
#include "text-format.h"
#include "TextShapeData.h"

#include "common/result.hpp"
#include "compat/basic-types.h"

// Forward declarations
namespace octopus {
    struct Text;
}
namespace textify {
    struct Context;
}

namespace textify {
namespace priv {

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

using TextShapeResult = Result<TextShapeDataPtr, TextShapeError>;
using TextDrawResult = Result<TextDrawOutput, TextDrawError>;

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
                        const FormattedText::FormattingParams &textParams,
                        float baseline,
                        void *pixels,
                        int width,
                        int height,
                        float scale,
                        bool dry,
                        const compat::Rectangle &viewArea);

TextDrawResult drawTextInner(Context &ctx,
                             bool dry, // Only compute the boundaries, the actual drawing does not take place.
                             const FormattedText::FormattingParams &textParams,
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
                                                OverflowPolicy overflowPolicy,
                                                int textWidth,
                                                RenderScale scale,
                                                VerticalPositioning &positioning,
                                                float &caretVerticalPos);

} // namespace priv
} // namespace textify
