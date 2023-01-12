#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include <map>

#include "textify-api.h"


namespace octopus {
    struct Text;
}

namespace textify {

struct Context;
struct TextShape;

/// NEW API:
/// TODO: Remove all unnecessary API state

// TODO: Matus: Remove and replace
enum class HorizontalAlign_NEW {
    DEFAULT,
    LEFT,
    RIGHT,
    CENTER,
    JUSTIFY,
    START,
    END
};

// TODO: Matus: Use the full font specifier (family and style?)
//struct FontSpecifier {
//    std::string postScriptName;
//    std::optional<std::string> family;
//    std::optional<std::string> style;
//};
using FontSpecifier = std::string;
struct Vector2d {
    double x, y;
};
struct PlacedGlyph {
    /// Glyph codepoint - index within the loaded font file
    uint32_t glyphCodepoint;
    /// Glyph position, specified by its four quad corners.
    struct QuadCorners {
        Vector2d topLeft, topRight, bottomLeft, bottomRight;
    } quadCorners;
    /// Glyph color
    uint32_t color;
    // TODO: Matus: This font face Id should be moved to some other place
    //   Maybe the glyphs should be groupped by face Ids
    std::string fontFaceId;
    /// Decoration - underline, strikethrough etc.
    enum class Decoration {
        NONE = 0,
        UNDERLINE,
        DOUBLE_UNDERLINE,
        STRIKE_THROUGH,
    };
    std::vector<Decoration> decorations;

    // TODO: Matus: Temporary data
    // TODO: Matus: Remove and replace
    struct Temporary {
        // Immediate format:
        // inherited: faceId, size;
        struct ImmediateFormat {
            // TODO: Matus: Inherited from GlyphsFormat
            float size;

            // TODO: Matus: ImmediateFormat
            float paragraphSpacing; // px in Sketch, cannot be negativ
            HorizontalAlign_NEW align;
        } format;

        float defaultLineHeight;

        /// Dimensions (scalable)
        struct Dimensions {
            float ascender;     //!< Up to 1px error, see FT reference
            float descender;    //!< Up to 1px error, see FT reference
            float lineHeight; // TODO why is this here and also in format?
        } dimensions;
    } temp;
};
using PlacedGlyphs = std::vector<PlacedGlyph>;

// TODO: Matus: Remove and replace
struct ShapeTextResult_NEW {
    PlacedGlyphs placedGlyphs;

    Matrix3f textTransform;
    FRectangle unstretchedTextBounds;
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
