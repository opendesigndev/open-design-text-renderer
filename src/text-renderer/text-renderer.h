
#pragma once

#include <open-design-text-renderer/PlacedTextData.h>

#include "errors.h"
#include "text-format.h"
#include "TextShapeData.h"

#include "../common/result.hpp"
#include "../compat/basic-types.h"

// Forward declarations
namespace octopus {
    struct Text;
}
namespace odtr {
    struct Context;
}

namespace odtr {
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

using TextShapeInputPtr = std::unique_ptr<TextShapeInput>;
using TextShapeResult = Result<TextShapeDataPtr, TextShapeError>;
using TextDrawResult = Result<TextDrawOutput, TextDrawError>;
using PlacedTextResult = Result<PlacedTextDataPtr, TextShapeError>;
using TextShapeParagraphsResult = std::pair<TextShapeResult, ParagraphShape::DrawResults>;


/// Compute drawn bitmap boundaries from the scaled placed text data bounds and the view are.
compat::Rectangle computeDrawBounds(Context &ctx,
                                    const PlacedTextData &placedTextData,
                                    float scale,
                                    const compat::Rectangle &viewArea);

/// List all font face names that have not been loaded to the context's FontManager.
FacesNames listMissingFonts(Context &ctx,
                            const octopus::Text& text);

TextShapeInputPtr preprocessText(Context &ctx,
                                 const octopus::Text &text);

TextShapeResult shapeText(Context &ctx,
                          const TextShapeInput &textShapeInput);

TextDrawResult drawText(Context &ctx,
                        const TextShapeInput &shapeInput,
                        const TextShapeData &shapeData,
                        float scale,
                        const compat::Rectangle& viewArea,
                        void* pixels, int width, int height,
                        bool dry); // Only compute the boundaries, the actual drawing does not take place.

TextDrawResult drawTextInner(Context &ctx,
                             const ParagraphShapes& paragraphShapes,
                             const FormattedText::FormattingParams &textParams,
                             const compat::FRectangle& unscaledTextBounds,
                             float baseline,
                             RenderScale scale,
                             const compat::FRectangle& viewArea,
                             Pixel32* pixels, int width, int height,
                             bool dry); // Only compute the boundaries, the actual drawing does not take place.

/// Draw individual ParagraphShapes.
ParagraphShape::DrawResults drawParagraphsInner(Context &ctx,
                                                const ParagraphShapes &shapes,
                                                OverflowPolicy overflowPolicy,
                                                int textWidth,
                                                RenderScale scale,
                                                VerticalPositioning positioning,
                                                BaselinePolicy baselinePolicy,
                                                float &caretVerticalPos);

PlacedTextResult shapePlacedText(Context &ctx,
                                 const TextShapeInput &textShapeInput);

// Draw text in the PlacedText representation into bitmap. Clip by viewArea.
TextDrawResult drawPlacedText(Context &ctx,
                              const PlacedTextData &placedTextData,
                              float scale,
                              const compat::Rectangle &viewArea,
                              void *pixels, int width, int height);

// Draw text in the PlacedText representation into bitmap. Clip by viewArea.
TextDrawResult drawPlacedTextInner(Context &ctx,
                                   const PlacedTextData &placedTextData,
                                   RenderScale scale,
                                   const compat::FRectangle& viewArea,
                                   Pixel32* pixels, int width, int height);

} // namespace priv
} // namespace odtr
