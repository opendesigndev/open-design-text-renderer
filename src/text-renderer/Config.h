#pragma once

namespace odtr {

namespace priv {

/**
 * Configure context dependent and unique Textify behaviour.
 *
 * @note If you ever think about `if (origin == ...)` then stop and put it here instead.
 */

struct Config
{
    Config() = default;

    // explicit Config(Document::Origin origin);

    /**
     * Baseline computed in floating point math is generally rounded to the nearest integer, however some Sketch
     * designs rather floor the baseline. When such example occurs, inspect the file version and possible auxiliary
     * flags set on particular layer.
     */
    bool floorBaseline = false;

    /**
     * Limit interlexical space width during justification. If the spaces between words cannot stretch any further,
     * the spaces between letters will.
     */
    bool limitJustifySpaceWidth = false;

    /**
     * Justify ambiguous lines, such as those ending with a soft break.
     */
    bool justifyAmbiguous = true;

    /**
     * Cut last line if not enough vertical space, see LastLinePolicy
     */
    // TODO: move to octopus into overflow line policy
    bool cutLastLine = true;

    /**
     * For Sketch the baseline is positioned in the middle of the first line.
     *
     * @see baseline policy
     */
    // bool shiftBaselineToMiddle = false;

    /**
     * Disregard bearing of the first glyph on a line. This effectively shifts the entire line a tiny bit in
     * the direction of its start.
     *
     * Although it's not clear what the actual value should be. Probably like bearing.x of one of the first
     * glyphs within a paragraph. This value should be constant per layer/paragraph.
     *
     * Better if this is disabled.
     */
    bool disregardFirstBearing = false;

    /**
     * Disregard additional spacing appended to the last glyph on a line.
     */
    bool disregardLastSpacing = false;

    /**
     * Render right to left text in the correct direction. Disable for layers with strings already reordered.
     */
    bool enableRtl = true;

    /**
     * Use TrueType kerning based on 'kern' table.
     */
    bool allowTrueTypeKerning = true;

    /**
     * Use 'kern' table kerning even if 'kern' feature in GPOS is available.
     */
    bool forceTrueTypeKerning = false;

    /**
     * Text can exceed its bounds indefinitely in vertical direction.
     */
    // TODO: move to octopus into overflow line policy
    bool infiniteVerticalStretch = false;

    /**
     * Text bounds are offset by last line's descender value (to fit parts of glyph below baseline).
     */
    bool lastLineDescenderOffset = true;

    /**
     * Actual line height is prefered over explictly set value, observable at cases when real height
     * is smaller than the explict line height.
     *
     * XD only behaviour.
     */
    bool preferRealLineHeightOverExplicit = false;

    /**
     * If enabled preprocess step will export outline for each glyph within the text.
     */
    bool exportOutlines = false;

    // INTERNAL CONFIG
    bool internalDisableHinting = true;

    bool enableViewAreaCutout = true;
};

}
} // namespace odtr
