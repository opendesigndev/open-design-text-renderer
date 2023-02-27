by @vadimpetrov

# Open Design Text Renderer

**Open Design Text Renderer** is a module responsible for text rendering. The input is formatted text with information about text styles, colors etc. and the output is either a transparent raster image or a collection of vector text outlines (experimental).

## Table of Contents

1. [Prerequisities](#prerequisities)
2. [Typography basics](#typography-basics)
    1. [Glyph metrics](#glyph-metrics)
    2. [Sizes and distances](#sizes-and-distances)
2. [Workflow](#workflow)
3. [Debugging](#debugging)
4. [Further reading](#further-reading)

## Prerequisities

**Open Design Text Renderer** is based on two widespread libraries from the (more or less) standard [text rendering stack][behdad]. On the bottom is font rasterizer [FreeType], on top the slightly more abstract text shaper [HarfBuzz].

**Why rasterize?** Fonts are most commonly represented as TrueType `ttf` or OpenType `otf` files. Both use tables mapping characters to their respective glyphs (encoding of 'a' to an image of 'a') and to various rules that apply to single glyphs and their combinations, such as hinting or kerning (see later). Both formats represent glyphs most commonly with vector outlines using line segments and Beziér curves (altough bitmap or svg fonts also exist), which need to be nontrivially rasterized. The many challenges in font rasterization lie in *hinting* – keeping vertical and horizontal lines visible even at small pixel sizes, aligning with the pixel grid, optimizing for LCDs etc.

**What's shaping?** Many type systems tend to join glyphs together. This is most prevalent in Arabic, Persian and similar languages, however examples can be found even in Latin scripts in the form of the so called *ligatures* such as 'ffj' or 'st' (your browser might not display either or both of those, but the book you're reading before bedtime generally will). Before the text can be rendered it needs to be shaped, or transformed to a sequence of special characters provided by the font, which are eventually turned into glyphs and rasterized.

## Typography basics

### Glyph metrics

A glyph is described by multiple metrics, some more and some less important in the context of **ODTR**. The following image is an edited version from the FreeType manual.

![Glyph Metrics][glyph]

Width and height give the basic dimensions of the glyph. The (horizontal) advance is generally wider than the width as it gives the distance from the origin of the glyph to the origin of the next glyph. This distance is sometimes later modified by *kerning* – the process of optimizing inter-character space for better reading and genearal `A E S T H E T I C S`. Typically kerned pairs of letter include 'VA', 'To' or 'ij'. If those look too far appart in your browser, then you have a case of [bad kerning][kerning].

The origin of the glyph lies on a baseline. The height can be split into two parts: a descender – height below the baseline, and an ascender – height above the baseline. This is especially important for placing the glyph vertically correctly, computing line height (distance between successive baselines) or text bounds. Be vary that the descender can be negative, ie how far *above* the baseline the glyph *descends*.

### Sizes and distances

This is perhaps the most confusing part of digital typography. Text size is generally given in *pixels* \[px\] or *points* \[pt\]. Since pixel size varies greatly between devices (compare LED walls to Retina displays) it is completely unsuitable to specify text size. Better is the more historic point as an 1/72 of an inch. Given display resolution *res* \[dpi\] the text size in pixels can be then computed as

*pixel_size  = point_size \* res / 72*

A glyph outline is drawn against a reference box called for historic purposes the *EM square*. A text size in points gives the size of this abstract box. Since the outline can be virtually any size – no need to actually fit the box, the same text rendered with different fonts can have wildly different dimensions. **It is generally not possible to tell how large a text will be from its font size.**

The EM square is divided into a grid. Typical TrueType fonts use an EM size of 2048 units; other fonts can use an EM size of 1000 grid units. To get pixel values, either look into the `size` table, use the `Face::scaleFontUnits` method or convert them as

*pixel_coord = grid_coord \* pixel_size / EM_size*

When you look into a FreeType face during debugging, you will find unscaled values (maximum advance, maximum ascender, ...) given in those units. The picture shows values from OpenSans Regular.

![Face Metrics][face]

FreeType also uses two numeric representations: the *16.16 fixed point* and *26.6 fixed point 1/64 fractional*. **Stay clear** of those. After working with them for about a year I still wonder about the whats abd whys. Whenever encountered study the docs closely and use the appropriate conversion methods in `FreetypeHandle`.

For more on font files, glyph metrics, measurements and other (but far from all!) digital typography mysteries see [FreeType Glyph Conventions][conventions].

## Workflow

**ODTR** has two main public methods which hide all the maelstrom within: `preprocess` and `drawText`. The idea is that each layer should be preprocessed exactly once and then drawn multiple times with different parameters such as rectangle cutout or scale factor. All data required for those procedures is supplied by a `odtr::Context`.

The text is first split into paragraphs which are then processed independently as `ParagraphShape` objects. Each paragraph is shaped by HarfBuzz and then drawn – converted to glyphs, but without any raster operations. Multiple values are computed during this phase such as the first ascender, first baseline, last descender and perhaps most importantly the text bounds. The entire result is saved in the context, which is later used during drawing.

`ParagraphShape` does most of the **ODTR** magic as it represents paragraphs with everything required to render them. Multiple operations take place while shaping a paragraph: analyzing line break opportunities in [bidirectional texts][bidi] using the International Components for Unicode lib, dividing paragraph into sequences with unfirom glyph format, shaping said sequences and eventually breaking them into lines. The result is a collection of `GlyphShape` objects with information about lines, text direction and glyph format and metrics.

The shapes are redrawn using a given scale factor into a `TypesetJournal` with records describing lines and text decorations (underline etc). During this the text is justified and the glyphs are translated to their position in the final bitmap, where they are eventually rendered.

## Debugging

**ODTR**, as all of Rendering does, suffers from a terrible case of GIGO. Many of the issues like *font not rendered correctly* or *text missing* come from bad data in Octopus such as mangled or missing text or formatting data. It is therefore a good idea to check the Octopus first and if there's anything wrong with it, it's someone else's job, yay!

Sometimes there is a problem with a font. Fonts are identified by their `PostScriptName` and often a single font can be known to different systems under various names, consider `Arial`, `ArialMT` or `ArialMS`. The font file provided can also contain a different subset of Unicode characters that is required by the text, usually out of the basic ASCII / English range – Hebrew, Cyrillic, Chinese etc. however this should be handled by a service independent on **ODTR** (Schriftmeister at the time of writing this document). On the other hand, different font files with the same `PostScriptName` can contain different values regarding single glyphs or the font in genearal. This can happen with later and earlier versions of the font from the same provider, with hacked fonts or with no apparent reason and is not really generally solvable.

Correctly rendered but misplaced text is usually a problem with bounds. Layer bounds sound simple enough, but they are actually a debugging evergreen and over time seem to get worse instead of better. Beware of editors that translate text to the start of the baseline instead of the top left corner (I'm looking at you, XD)!

Some editors offer automatic computation of bounds and it does happen that **ODTR** comes up with slightly different values. There has been a lot of discussion on this, sometimes it has been fixed and a few times deemed unsolvable or marked as a feature instead of a bug.

Longer texts with multiple lines commonly end up with different line breaking compared to the original – meaning a word can be at the start of the next line instead of at the end of the current one or the other way around. This problem is considered unavoidable because of the many factors affecting horizontal placement of a glyph, including character spacing, kerning, glyph advances and bearings, font differences (and fallback fonts!), hinting, sub-pixel positioning, limited precision arithmetic, rounding errors... and just one pixel difference can force the word to shift. This does not constitue a real problem for longer texts since they are mostly Lorem ipsum anyway, but affects single lines greatly. Those need to be approached with care.

For most other problems the best approach is top down. Check if **ODTR** produces any images, how many `ParagraphShape` objects are created, what values glyphs get during shaping... There are many warnings to help you locate a problem and more often than not, they will repeat a couple of times making them rather hard to miss, just set the log level.

## Further reading

- read the [FreeType Glyph Conventions][conventions]
- check [FreeType docs][ftdocs]
- get mad at incomplete [HarfBuzz docs][hbdocs]
- check out the suprisingly extensive [OpenType spec][otspec] published by Microsoft :open_mouth:
- learn the [Greek alphabet][greek]
- check out some cool stuff like [Cyrillic numerals][cyrillic], [font basics][fonts] or [Storm Type Foundry][storm]

[behdad]: http://behdad.org/text/
[freetype]: https://www.freetype.org
[harfbuzz]: https://harfbuzz.github.io
[kerning]: https://www.xkcd.com/1015/
[glyph]: img/glyphmetrics.png "Glyph Metrics"
[face]: img/facemetrics.png "Face Metrics"
[conventions]: https://www.freetype.org/freetype2/docs/glyphs/
[ftdocs]: https://www.freetype.org/freetype2/docs/reference/ft2-index.html
[hbdocs]: https://harfbuzz.github.io/
[otspec]: https://docs.microsoft.com/en-us/typography/opentype/spec/
[bidi]: https://www.unicode.org/reports/tr9/
[cyrillic]: https://en.wikipedia.org/wiki/Cyrillic_script#Letters
[storm]: https://www.stormtype.com/
[greek]: https://en.wikipedia.org/wiki/Greek_alphabet
[fonts]: https://www.jotform.com/blog/a-crash-course-in-typography-the-basics-of-type/
