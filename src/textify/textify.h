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

class FormattedText;
class ParagraphShape;

using UsedFaces = std::unordered_set<std::string>;
using FormattedTextPtr = std::unique_ptr<FormattedText>;

using MayBeFrameSize = std::optional<compat::Vector2f>;

struct TextShapeData
{
    TextShapeData(FormattedTextPtr text,
                  MayBeFrameSize frameSize,
                  const compat::Matrix3f& textTransform,
                  ParagraphShapes&& shapes,
                  const compat::FRectangle& boundsNoTransform,
                  const compat::FRectangle& boundsTransformed,
                  float baseline);

    // input properties
    FormattedTextPtr formattedText;
    MayBeFrameSize frameSize;
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

/// List all font face names that have not been loaded to the context's FontManager.
FacesNames listMissingFonts(Context* ctx, const octopus::Text& text);

TextShapeResult shapeText(Context* ctx, const octopus::Text& text);

TextShapeResult reshapeText(Context* ctx, TextShapeDataPtr&& textShapeData);

TextShapeResult shapeTextInner(Context& ctx,
                FormattedTextPtr text,
                const std::optional<compat::Vector2f>& frameSize,
                const compat::Matrix3f& textTransform
                );

TextDrawResult drawText(Context* ctx,
                        const TextShapeData& shapedData,
                        void* pixels,
                        int width,
                        int height,
                        float scale,
                        bool dry,
                        const compat::Rectangle& viewArea);

TextDrawResult drawTextInner(
                    Context& ctx,
                    bool dry,

                    const FormattedText& text,
                    const compat::FRectangle& unscaledTextBounds,
                    float baseline,

                    RenderScale scale,
                    const compat::FRectangle& viewArea,
                    bool alphaMask,

                    const ParagraphShapes& paragraphShapes,
                    Pixel32* pixels,
                    int width,
                    int height);

/// Draw individual ParagraphShapes.
ParagraphShape::DrawResults drawParagraphsInner(const ParagraphShapes &shapes,
                                                Context &ctx,
                                                const FormattedText &text,
                                                int textWidth,
                                                RenderScale scale,
                                                bool isAlphaMask,
                                                VerticalPositioning &positioning,
                                                float &caretVerticalPos);

} // namespace priv
} // namespace textify
