
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
                                                VerticalPositioning positioning,
                                                float &caretVerticalPos);





struct PlacedGlyph_pr {
    /// Glyph codepoint - index within the loaded font file
    uint32_t glyphCodepoint;
    /// Glyph position, specified by its four quad corners.
    struct QuadCorners {
        compat::Vector2f topLeft, topRight, bottomLeft, bottomRight;
    } quadCorners;
    /// Glyph color
    uint32_t color;
    // TODO: Matus: This font face Id should be moved to some other place
    //   Maybe the glyphs should be groupped by face Ids
    std::string fontFaceId;

    // TODO: Matus: This should not be here at all
    struct Temp {
        float size = 0.0f;
    } temp;
};
using PlacedGlyphs_pr = std::vector<PlacedGlyph_pr>;

struct PlacedDecoration_pr {
    /// Decoration - underline, strikethrough etc.
    enum class Type {
        NONE = 0,
        UNDERLINE,
        DOUBLE_UNDERLINE,
        STRIKE_THROUGH,
    } type;

    struct
    {
        int first, last;
    } xRange; ///< horizontal range in pixels

    Pixel32 color;
    int yOffset;
    float thickness;
};
using PlacedDecorations_pr = std::vector<PlacedDecoration_pr>;

using UsedFaces = std::unordered_set<std::string>;
using FrameSizeOpt = std::optional<compat::Vector2f>;


struct TextShapeData_NEW
{
    TextShapeData_NEW(FormattedTextPtr text,
                      FrameSizeOpt frameSize,
                      const compat::Matrix3f& textTransform,
                      ParagraphShapes&& shapes,
                      const compat::FRectangle& boundsNoTransform,
                      const compat::FRectangle& boundsTransformed,
                      float baseline,
                      const PlacedGlyphs_pr &placedGlyphs,
                      const PlacedDecorations_pr &placedDecorations,
                      const compat::FRectangle unstretchedTextBounds)
        : formattedText(std::move(text)),
          frameSize(frameSize),
          textTransform(textTransform),
          usedFaces(formattedText->collectUsedFaceNames()),
          paragraphShapes(std::move(shapes)),
          textBoundsNoTransform(boundsNoTransform),
          textBoundsTransformed(boundsTransformed),
          baseline(baseline),
          placedGlyphs(placedGlyphs),
          placedDecorations(placedDecorations),
          unstretchedTextBounds(unstretchedTextBounds)
    {
    }

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

    // TODO: Matus
    PlacedGlyphs_pr placedGlyphs;
    PlacedDecorations_pr placedDecorations;
    compat::FRectangle unstretchedTextBounds;
};
using TextShapeDataPtr_NEW = std::unique_ptr<TextShapeData_NEW>;
using TextShapeResult_NEW = Result<TextShapeDataPtr_NEW, TextShapeError>;


TextShapeResult_NEW shapeText_NEW(Context &ctx,
                                  const octopus::Text& text);

TextShapeResult_NEW shapeTextInner_NEW(Context &ctx,
                                       FormattedTextPtr text,
                                       const FrameSizeOpt &frameSize,
                                       const compat::Matrix3f &textTransform);

compat::FRectangle getStretchedTextBounds(Context &ctx,
                                          const ParagraphShapes &paragraphShapes,
                                          const compat::FRectangle &unscaledTextBounds,
                                          const FormattedText::FormattingParams &textParams,
                                          float baseline,
                                          float scale);

compat::Rectangle computeDrawBounds(Context &ctx,
                                    const compat::Matrix3f &textTransform,
                                    const compat::FRectangle &stretchedTextBounds,
                                    float scale,
                                    const compat::FRectangle& viewAreaTextSpace);

// TODO: Matus: Cleanup this function - make sure all the arguments are optimal etc.
TextDrawResult drawText_NEW(Context &ctx,
                            const compat::FRectangle &stretchedTextBounds,
                            void *pixels, int width, int height,
                            float scale,
                            const compat::Rectangle &viewArea,
                            const PlacedGlyphs_pr &placedGlyphs,
                            const PlacedDecorations_pr &placedDecorations);

TextDrawResult drawTextInner_NEW(Context &ctx,
                                 RenderScale scale,
                                 const compat::FRectangle& viewArea,
                                 Pixel32* pixels, int width, int height,
                                 const PlacedGlyphs_pr &placedGlyphs,
                                 const PlacedDecorations_pr &placedDecorations);

} // namespace priv
} // namespace textify
