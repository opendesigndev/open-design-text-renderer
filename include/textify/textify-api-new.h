
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "textify-api.h"


namespace octopus {
    struct Text;
}

namespace textify {

struct Context;
struct TextShape;
typedef Context* ContextHandle;
typedef TextShape* TextShapeHandle;

struct Vector2f {
    float x, y;
};

struct PlacedGlyph {
    /// Glyph codepoint - index within the loaded font file
    uint32_t glyphCodepoint;
    /// Glyph position, specified by its four quad corners.
    struct QuadCorners {
        Vector2f topLeft, topRight, bottomLeft, bottomRight;
    } quadCorners;
    /// Glyph color
    uint32_t color;
    // TODO: Matus: This font face Id should be moved to some other place
    //   Maybe the glyphs should be groupped by face Ids
    std::string fontFaceId;

    // TODO: Matus: Temporary data
    // TODO: Matus: Remove and replace
    struct Temporary {
        float size = 0.0f;
    } temp;
};
using PlacedGlyphs = std::vector<PlacedGlyph>;

struct PlacedDecoration {
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

    uint32_t color;
    int yOffset;
    float thickness;
};
using PlacedDecorations = std::vector<PlacedDecoration>;

struct ShapeTextResult_NEW {
    PlacedGlyphs placedGlyphs;
    PlacedDecorations placedDecorations;

    // TODO: Matus: Remove and replace -> this should be just PlacedGlyphs
    FRectangle textBounds;
};


/// Shape text.
TextShapeHandle shapeText_NEW(ContextHandle ctx,
                              const octopus::Text& text);
ShapeTextResult_NEW shapeText_NEW_Inner(ContextHandle ctx,
                                        const octopus::Text &text);

/// Draw text.
DrawTextResult drawText_NEW(ContextHandle ctx,
                            TextShapeHandle textShape,
                            void* pixels, int width, int height,
                            const DrawOptions& drawOptions = {});
DrawTextResult drawText_NEW_Inner(ContextHandle ctx,
                                  const ShapeTextResult_NEW &textShape_NEW,
                                  void *outputBuffer, int width, int height,
                                  const DrawOptions &drawOptions);
}
