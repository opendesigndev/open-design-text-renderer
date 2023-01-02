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
// TODO: Matus: Remove and replace
enum class Decoration_NEW {
    NONE = 0,
    UNDERLINE,
    DOUBLE_UNDERLINE,
    STRIKE_THROUGH,
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
    /// Color - TODO: Matus: this is duplicated in ImmediateFormat
    uint32_t color;

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
            Decoration_NEW decoration;
            HorizontalAlign_NEW align;
        } format;

        // TODO: Matus: This font face Id should be moved to some other place
        //   Maybe the glyphs should be groupped by face Ids
        std::string fontFaceId;
        float defaultLineHeight;

        /// Dimensions (scalable)
        struct Dimensions {
            float horizontalAdvance;
            float ascender;     //!< Up to 1px error, see FT reference
            float descender;    //!< Up to 1px error, see FT reference
            float lineHeight; // TODO why is this here and also in format?
        } dimensions;
    } temp;
};
using PlacedGlyphs = std::vector<PlacedGlyph>;

// TODO: Matus: Remove and replace
struct VisualRun_NEW {
    long start, end;
    bool leftToRight;
    float width;
};

// TODO: Matus: Remove and replace
enum class Justifiable_NEW {
    POSITIVE,
    NEGATIVE,
    DOCUMENT
};

// TODO: Matus: Remove and replace
struct Line_NEW {
    long start, end;
    //! BiDi visual runs.
    std::vector<VisualRun_NEW> visualRuns;
    float lineWidth;
    bool baseDirLeftToRight = true;
    Justifiable_NEW justifiable;
};
using Line_NEW_vec = std::vector<Line_NEW>;

// TODO: Matus: Remove and replace
struct Paragraph_NEW {
    PlacedGlyphs glyphs_;
    Line_NEW_vec lines_;
};
using Paragraph_NEW_vec = std::vector<Paragraph_NEW>;

// TODO: Matus: Remove and replace
struct ShapeTextResult_NEW {
    Paragraph_NEW_vec paragraphs;
    Matrix3f textTransform;
    FRectangle textBoundsNoTransform;

    struct TextParams {
        enum class VerticalAlign {
            TOP,
            CENTER,
            BOTTOM
        } verticalAlign;

        enum class BoundsMode {
            AUTO_WIDTH,    //!< bounds extended horizontally
            AUTO_HEIGHT,   //!< fixed width, bounds extended vertically
            FIXED,         //!< bounds of a fixed size
        } boundsMode;

        float baseline = 0.0f;

        enum class HorizontalPositionPolicy {
            ALIGN_TO_FRAME,
            FRAME_AS_HINT,
        } horizontalPolicy;

        enum class BaselinePolicy {
            SET,
            CENTER,
            OFFSET_ASCENDER,
            OFFSET_BEARING
        } baselinePolicy;

        enum class OverflowPolicy {
            NO_OVERFLOW,
            CLIP_LINE,
            EXTEND_LINE,
            EXTEND_ALL
        } overflowPolicy;
    } textParams;
};


/// Shape text.
ShapeTextResult_NEW shapeText_NEW(ContextHandle ctx,
                                  const octopus::Text &text);

/// Draw text.
DrawTextResult drawText_NEW(ContextHandle ctx,
                            const ShapeTextResult_NEW &textShape_NEW,
                            void *outputBuffer, int width, int height,
                            const DrawOptions &drawOptions);
}
