#include "TypesetJournal.h"

#include <algorithm>

namespace textify {
namespace priv {

using namespace compat;

void TypesetJournal::startLine(float y)
{
    lineJournal_.push_back({});

    baselines_.push_back(y);
}

void TypesetJournal::addGlyph(GlyphPtr glyph)
{
    if (lineJournal_.empty()) {
        lineJournal_.push_back({});
    }

    lineJournal_.back().glyphJournal_.emplace_back(std::move(glyph));
}

void TypesetJournal::addDecoration(const DecorationInput& d, float scale)
{
    extendLastDecoration(d.end.x, d.type);

    static const auto STRIKETHROUGH_HEIGHT = 0.25f;

    DecorationRecord decoration;

    decoration.range = {d.start.x, d.end.x};
    decoration.type = d.type;
    decoration.color = d.color;

    decoration.offset = d.start.y;
    auto decorOffset = 0.0f;
    if (d.type == Decoration::STRIKE_THROUGH) {
        decorOffset =
            (STRIKETHROUGH_HEIGHT * d.face->scaleFontUnits(d.face->getFtFace()->height, true));
    } else {
        decorOffset = (d.face->scaleFontUnits(d.face->getFtFace()->underline_position, true));
    }

    decoration.offset -= static_cast<int>(decorOffset * scale);

    auto thickness = d.face->scaleFontUnits(d.face->getFtFace()->underline_thickness, true);
    decoration.thickness = thickness * scale;

    decorationJournal_.push_back(decoration);
}

void TypesetJournal::extendLastDecoration(int newRange, Decoration type)
{
    if (decorationJournal_.empty() || decorationJournal_.back().type != type) {
        return;
    }

    decorationJournal_.back().range.last = newRange;
}

TypesetJournal::DrawResult TypesetJournal::draw(BitmapRGBA& bitmap, const Rectangle& bounds, int textHeight, const Rectangle& viewArea, const Vector2i& offset) const
{
    auto drawResult = drawGlyphs(bitmap, bounds, textHeight, viewArea, offset);

    drawDecorations(bitmap, offset);

    return drawResult;
}

Rectangle TypesetJournal::stretchedBounds(int yOffset, int yMax) const
{
    Rectangle bounds = {};

    for (const auto& line : lineJournal_) {

        auto lineExtremes = line.glyphBitmapExtremes();

        if (yMax > 0 && lineExtremes.high > yMax) {
            // line upper glyph bound is below a visble limit
            // line isn't visible at all, stop stretching
            break;
        }

        if (lastLinePolicy_ == LastLinePolicy::CUT && lineExtremes.partialOverflow(yMax)) {
            break;
        }

        for (const auto& rec : line.glyphJournal_) {
            auto glyphBounds = rec->getBitmapBounds();

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
    // auto t0 = Timer::now();
    auto numGlyphsRendered = 0ul;

    for (const auto& line : lineJournal_) {

        if (lastLinePolicy_ == LastLinePolicy::CUT && line.lowPoint() + offset.y > textHeight)
            break;

        for (const auto& rec : line.glyphJournal_) {
            auto glyphBounds = rec->getBitmapBounds();
            auto placedGlyphBounds = bounds + glyphBounds;
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

    // t0.stop();

    return DrawResultData{numGlyphsRendered /* , std::move(t0) */};
}

class BmpWriter
{
public:
    explicit BmpWriter(BitmapRGBA& bitmap)
        : bitmap_(bitmap)
    { }

    bool checkV(int y) {
        return y >= 0 && y < bitmap_.height();
    }

    bool checkH(int x) {
        return x >= 0 && x < bitmap_.width();
    }

    void write(int x, int y, Pixel32 color) {
        if (checkV(y)) {
            bitmap_.pixels()[y * bitmap_.width() + x] = color;
        }
    }
private:
    BitmapRGBA& bitmap_;
};

void TypesetJournal::drawDecorations(BitmapRGBA& bitmap, const Vector2i& offset) const
{
    auto w = BmpWriter(bitmap);

    for (const auto& decoration : decorationJournal_) {

        auto vpos = decoration.offset + offset.y;
        for (int j = 0; j < decoration.range.last - decoration.range.first; j++) {
            auto penX = decoration.range.first + j + offset.x;
            if (!w.checkH(penX)) {
                continue;
            }

            if (decoration.type == Decoration::DOUBLE_UNDERLINE) {
                auto thickness = static_cast<int>(ceil(decoration.thickness * 2.0 / 3.0));
                auto offset = static_cast<int>(thickness * 0.5);

                for (auto k = 0; k < thickness; k++) {
                    w.write(penX, vpos - offset - k, decoration.color);
                    w.write(penX, vpos + offset + k, decoration.color);
                }
            } else /* UNDERLINE || STRIKETHROUGH */ {
                for (auto k = 0; k < decoration.thickness; k++) {
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
    for (const auto& rec : glyphJournal_) {
        low = std::max(low, rec->getDestination().y + rec->bitmapHeight());
    }

    return low;
}

TypesetJournal::LowHighPair TypesetJournal::LineRecord::glyphBitmapExtremes() const
{
    auto high = std::numeric_limits<int>::max();
    auto low = 0;

    for (const auto& rec : glyphJournal_) {
        high = std::min(high, rec->getDestination().y);
        low = std::max(low, rec->getDestination().y + rec->bitmapHeight());
    }

    return {high, low};
}

} // namespace priv
} // namespace textify
