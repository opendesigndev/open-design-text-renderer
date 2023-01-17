
#include "TypesetJournal.h"

#include <algorithm>
#include <cmath>

#include "BmpWriter.h"

namespace textify {
namespace priv {

using namespace compat;

void TypesetJournal::startLine(float y)
{
    lineJournal_.push_back({});
}

void TypesetJournal::addGlyph(GlyphPtr glyph, const Vector2f &offset)
{
    if (lineJournal_.empty()) {
        lineJournal_.push_back({});
    }

    lineJournal_.back().glyphJournal_.emplace_back(std::move(glyph));
    lineJournal_.back().offsets_.emplace_back(offset);
}

void TypesetJournal::addDecoration(const DecorationInput& d, float scale)
{
    extendLastDecoration(d.start.x, d.type);

    static const float STRIKETHROUGH_HEIGHT = 0.25f;

    DecorationRecord decoration;

    decoration.range = {d.start.x, d.end.x};
    decoration.type = d.type;
    decoration.color = d.color;

    const float decorOffset = (d.type == Decoration::STRIKE_THROUGH)
        ? (STRIKETHROUGH_HEIGHT * d.face->scaleFontUnits(d.face->getFtFace()->height, true))
        : (d.face->scaleFontUnits(d.face->getFtFace()->underline_position, true));

    decoration.offset = d.start.y - static_cast<int>(decorOffset * scale);

    const float thickness = d.face->scaleFontUnits(d.face->getFtFace()->underline_thickness, true);
    decoration.thickness = thickness * scale;

    decorationJournal_.push_back(decoration);
}

void TypesetJournal::extendLastDecoration(int newRange, Decoration type)
{
    // TODO: Matus: This is nasty and it doesn't work as expected. In case there is a break in decorations
    //   (a chunk of text is not underlined) then this undelines the whole text
    if (!decorationJournal_.empty() && decorationJournal_.back().type == type) {
        decorationJournal_.back().range.last = newRange;
    } else if (decorationJournal_.size() >= 2 && decorationJournal_[decorationJournal_.size()-2].type == type) {
        decorationJournal_[decorationJournal_.size()-2].range.last = newRange;
    }
}

TypesetJournal::DrawResult TypesetJournal::draw(BitmapRGBA& bitmap, const Rectangle& bounds, int textHeight, const Rectangle& viewArea, const Vector2i& offset) const
{
    const DrawResult drawResult = drawGlyphs(bitmap, bounds, textHeight, viewArea, offset);

    drawDecorations(bitmap, offset);

    return drawResult;
}

Rectangle TypesetJournal::stretchedBounds(int yOffset, int yMax) const
{
    Rectangle bounds = {};

    for (const LineRecord& line : lineJournal_) {
        const LowHighPair lineExtremes = line.glyphBitmapExtremes();

        if (yMax > 0 && lineExtremes.high > yMax) {
            // line upper glyph bound is below a visble limit
            // line isn't visible at all, stop stretching
            break;
        }

        if (lastLinePolicy_ == LastLinePolicy::CUT && lineExtremes.partialOverflow(yMax)) {
            break;
        }

        for (const GlyphPtr& rec : line.glyphJournal_) {
            Rectangle glyphBounds = rec->getBitmapBounds();
            bounds = bounds | glyphBounds;
        }

        if (yMax > 0 && lineExtremes.low > yMax) {
            // line botom glyph bound is below a visble limit
            // this was the last line visible, stop stretching
            break;
        }
    }

    bounds.t += yOffset;

    return bounds;
}

TypesetJournal::DrawResult TypesetJournal::drawGlyphs(BitmapRGBA& bitmap, const Rectangle& bounds, int textHeight, const Rectangle& viewArea, const Vector2i& offset) const
{
    size_t numGlyphsRendered = 0ul;

    for (const LineRecord &line : lineJournal_) {
        if (lastLinePolicy_ == LastLinePolicy::CUT && line.lowPoint() + offset.y > textHeight)
            break;

        for (const GlyphPtr& rec : line.glyphJournal_) {
            const Rectangle glyphBounds = rec->getBitmapBounds();
            Rectangle placedGlyphBounds = bounds + glyphBounds;
            placedGlyphBounds.l += offset.x;
            placedGlyphBounds.t += offset.y;

            if (!(placedGlyphBounds & viewArea)) {
                // skips glyphs outside of view area
                continue;
            }

            IDims2 destDims{bitmap.width(), bitmap.height()};
            rec->blit(bitmap.pixels(), destDims, offset);

            ++numGlyphsRendered;
        }
    }

    return DrawResultData { numGlyphsRendered };
}

void TypesetJournal::drawDecorations(BitmapRGBA& bitmap, const Vector2i& offset) const
{
    BmpWriter w = BmpWriter(bitmap);

    for (const DecorationRecord &decoration : decorationJournal_) {
        const int vpos = decoration.offset + offset.y;

        for (int j = 0; j < decoration.range.last - decoration.range.first; j++) {
            const int penX = decoration.range.first + j + offset.x;
            if (!w.checkH(penX)) {
                continue;
            }

            if (decoration.type == Decoration::DOUBLE_UNDERLINE) {
                const int thickness = static_cast<int>(ceil(decoration.thickness * 2.0 / 3.0));
                const int offset = static_cast<int>(thickness * 0.5);

                for (int k = 0; k < thickness; k++) {
                    w.write(penX, vpos - offset - k, decoration.color);
                    w.write(penX, vpos + offset + k, decoration.color);
                }
            } else /* UNDERLINE || STRIKETHROUGH */ {
                for (int k = 0; k < decoration.thickness; k++) {
                    w.write(penX, vpos - k, decoration.color);
                }
            }
        }
    }
}

bool TypesetJournal::LowHighPair::partialOverflow(int limit) const
{
    return high <= limit && low > limit;
}

int TypesetJournal::LineRecord::lowPoint() const
{
    int low = 0;

    for (const GlyphPtr &rec : glyphJournal_) {
        const IPoint2 &pos = rec->getDestination();
        low = std::max(low, pos.y + rec->bitmapHeight());
    }

    return low;
}

TypesetJournal::LowHighPair TypesetJournal::LineRecord::glyphBitmapExtremes() const
{
    int high = std::numeric_limits<int>::max();
    int low = 0;

    for (const GlyphPtr &rec : glyphJournal_) {
        const IPoint2 &pos = rec->getDestination();
        high = std::min(high, pos.y);
        low = std::max(low, pos.y + rec->bitmapHeight());
    }

    return {high, low};
}

} // namespace priv
} // namespace textify
