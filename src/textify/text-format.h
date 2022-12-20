// compat: core/src/model/text-format.h

#pragma once

// #include "../base/Bitmap.hpp"
// #include "../base/basic-types.h"
#include "../common/sorted_vector.hpp"
#include <string>
#include <vector>

typedef float font_size;
typedef float spacing;
typedef std::string FaceId;

/// A 32-bit RGBA pixel, with LE alignment (0xAABBGGRR)
typedef std::uint32_t Pixel32;

enum class HorizontalAlign
{
    DEFAULT,
    LEFT,
    RIGHT,
    CENTER,
    JUSTIFY,
    START,
    END
};

enum class VerticalAlign
{
    TOP,
    CENTER,
    BOTTOM
};

enum class TextDirection
{
    LEFT_TO_RIGHT = 0,
    RIGHT_TO_LEFT
};

enum class Decoration
{
    NONE = 0,
    UNDERLINE,
    DOUBLE_UNDERLINE,
    STRIKE_THROUGH,
};

enum class VerticalPositioning
{
    BASELINE,
    PREV_BASELINE,
    TOP_BOUND
};

enum class BoundsMode
{
    AUTO_WIDTH,    //!< bounds extended horizontally
    AUTO_HEIGHT,   //!< fixed width, bounds extended vertically
    FIXED,         //!< bounds of a fixed size
};

//! Whether to draw the last line if it does not entirely fit the target bitmap
enum class LastLinePolicy
{
    FORCE, //!< Draw
    CUT,   //!< Do not draw
};

/**
 * Describes ways of positioning the text horizontally in respect to the
 * provided frame.
 */
enum class HorizontalPositionPolicy
{
    /**
     * Text horizontal position starts in the top left corner of the provided
     * frame.
     *
     * @note    This is default behaviour suitable for most of the designs.
     */
    ALIGN_TO_FRAME,

    /**
     * Uses provided frame as a hint, the exact position depends on the text
     * alignment.
     *
     * @note    This behaviour is used to account for texts whose length
     *          is different to what the original frame computation expected.
     *          This is mostly used for text overrides.
     */
    FRAME_AS_HINT,
};

enum class BaselinePolicy
{
    /**
     * The first baseline is centered vertically within the first line,
     * therefore its position depends on line spacing.
     **/
    CENTER,

    /**
     * The first baseline is precisely given by transform matrix.
     **/
    SET,

    /**
     * The first baseline is shifted by the height of an ascender line, its
     * position only depends on font size, not the line spacing.
     **/
    OFFSET_ASCENDER,

    /**
     * The first baseline is shifted by the largest Y-axis bearing value at that line.
     */
    OFFSET_BEARING
};

/**
 * Describes behaviour of a text that overflows its frame.
 *
 * Applicable only to 'FIXED' text frame.
 */
enum class OverflowPolicy
{
    /**
     * Text strictly clipped by the frame.
     */
    NO_OVERFLOW,

    /**
     * The last line which doesn't fit the frame is entirely clipped.
     */
    CLIP_LINE,

    /**
     * Visible text is extended by the last line that at least partially fits the frame.
     */
    EXTEND_LINE,

    /**
     * The whole text overflow is visible.
     */
    EXTEND_ALL
};


/// Textify representation of octopus OpenType features.
struct TypeFeature
{
    std::string tag;
    std::int32_t value;
};
typedef std::vector<TypeFeature> TypeFeatures;

bool operator==(const TypeFeature& a, const TypeFeature& b);

typedef sorted_vector<spacing> TabStops;

/// Format properties that affect the bare render of an individual glyph
struct GlyphFormat
{
    /// Textify representation of octopus Ligatures enum
    enum class Ligatures
    {
        NONE = 0,
        STANDARD = 1,
        ALTERNATIVE = 2, ///< Inspired by PS behaviour, stands for discretionary and historical
        ALL = 3
    };

    FaceId faceId;
    font_size size;
    Ligatures ligatures;
    TextDirection direction;
    TypeFeatures features;
    TabStops tabStops;

    bool operator==(const GlyphFormat& b) const
    {
        return
            faceId == b.faceId &&
            size == b.size &&
            ligatures == b.ligatures &&
            direction == b.direction &&
            features == b.features &&
            tabStops == b.tabStops;
    }
};



/// Immediate format of a single character / glyph
struct ImmediateFormat : GlyphFormat
{
    // inherited: faceId, size;
    spacing lineHeight;       // px, always positive
    spacing minLineHeight;    // px
    spacing maxLineHeight;    // px
    spacing letterSpacing;    // px in Sketch, can be negative
    spacing paragraphSpacing; // px in Sketch, cannot be negativ
    spacing paragraphIndent;  // px x offset of the first line in a paragraph
    Pixel32 color;
    Decoration decoration;
    HorizontalAlign align;
    bool kerning;
    bool uppercase;
    bool lowercase;
};

/// Specifies a change in format for the substring in the range [start, end)
struct FormatModifier
{
    static const int FACE;
    static const int SIZE;
    static const int LINE_HEIGHT;
    static const int LETTER_SPACING;
    static const int PARAGRAPH_SPACING;
    static const int COLOR;
    static const int DECORATION;
    static const int ALIGN;
    static const int KERNING;
    static const int LIGATURES;
    static const int UPPERCASE;
    static const int LOWERCASE;
    static const int PARAGRAPH_INDENT;
    static const int TYPE_FEATURE;
    static const int TAB_STOPS;

    struct
    {
        int start, end;
    } range;

    int types;
    FaceId face;
    font_size size;
    spacing lineHeight;
    spacing minLineHeight;
    spacing maxLineHeight;
    spacing letterSpacing;
    spacing paragraphSpacing;
    spacing paragraphIndent;
    Pixel32 color;
    Decoration decoration;
    HorizontalAlign align;
    bool kerning;
    GlyphFormat::Ligatures ligatures;
    bool uppercase;
    bool lowercase;
    TypeFeatures features;
    TabStops tabStops;
};
