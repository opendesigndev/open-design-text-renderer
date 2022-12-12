#include "ParagraphShape.h"

#include "Glyph.h"
#include "tabstops.h"

#include "fonts/FontManager.h"
#include "utils/Log.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#define MAX_SPACE_COEF 2.8f // arbitrary

namespace textify {
namespace priv {

using namespace compat;

namespace {
inline static bool checkOverwrite(const BitmapRGBA& bitmap, const int pos)
{
    return pos < bitmap.width() * bitmap.height() && pos > 0 && bitmap.pixels()[pos] == 0 &&
           (pos + 1) % bitmap.width() != 0 && bitmap.pixels()[pos + 1] == 0 && (pos - 1) % bitmap.width() != 0 &&
           bitmap.pixels()[pos + 1] == 0;
}

static bool isSkipGlyph(qchar c)
{
    const std::vector<qchar> skipped = {
        0x200D, // Zero-width joiner
        /*
        0x1f3fB,
        0x1f3fC,
        0x1f3fD,
        0x1f3fE,
        0x1f3fF // Fitzpatrick skin modifiers
         */
    };

    return std::find(skipped.begin(), skipped.end(), c) != skipped.end();
}

static float evaluateAlign(HorizontalAlign align, TextDirection direction)
{
    switch (align) {
        case HorizontalAlign::LEFT:
            return 0.0f;
        case HorizontalAlign::RIGHT:
            return 1.0f;
        case HorizontalAlign::CENTER:
            return 0.5f;
        case HorizontalAlign::START:
            return direction == TextDirection::RIGHT_TO_LEFT ? 1.0f : 0.0f;
        case HorizontalAlign::END:
            return direction == TextDirection::RIGHT_TO_LEFT ? 0.0f : 1.0f;
        default:
            return 0.0f;
    }
}

static spacing lineHeightFromFace(const FacePtr face)
{
    if (face->isScalable()) {
        return face->scaleFontUnits(face->getFtFace()->height, true);
    } else {
        return FreetypeHandle::from26_6fixed(face->getFtFace()->size->metrics.height);
    }
}
}


ParagraphShape::ShapeResult::ShapeResult(bool success) : success(success) {}

void ParagraphShape::ReportedFaces::merge(const ReportedFaces& other)
{
    for (const auto& otherItem : other) {
        insert(otherItem);
    }
}


ParagraphShape::ParagraphShape(const utils::Log& log, const FontManager& fontManager, bool loadBearing)
    : log_(log), fontManager_(fontManager), loadBearing_(loadBearing)
{ }

ParagraphShape::ShapeResult ParagraphShape::shape(const FormattedParagraph& paragraph, float width)
{
    if (paragraph.text_.size() != paragraph.format_.size()) {
        log_.warn("Paragraph shaping error: Text format incorrectly expanded.");
        return false;
    }

    visualRuns_ = paragraph.visualRuns_;
    baseDirection_ = paragraph.baseDirection_;
    // Prepare Unicode line break opportunities
    lineStarts_ = LineBreaker::analyzeBreaks(log_,  paragraph.text_);

    ShapeResult result(false);
    auto textLen = static_cast<int>(paragraph.text_.size());
    auto pos = 0;

    // Divide paragraph into sequences with uniform glyph format
    while (pos < textLen) {
        Sequence seq;
        seq.start = pos;
        seq.len = 1;
        seq.format = paragraph.format_[pos];
        ++pos;

        while (pos < textLen && paragraph.format_[pos] == seq.format) {
            ++pos, ++seq.len;
        }

        shapeSequence(seq, paragraph, fontManager_.facesTable(), result);
    }

    LineBreaker breaker{log_, glyphs_, visualRuns_, baseDirection_};
    breaker.breakLines(int(width));
    const auto& lineSpans = breaker.getLines();
    if (lineSpans.empty()) {
        log_.warn("Paragraph breaking error: No lines present.");
        return result;
    }

    lineSpans_ = breaker.moveLines();

    result.success = !glyphs_.empty();
    return result;
}

ParagraphShape::DrawResult ParagraphShape::draw(const Context& ctx,
                                                int left,
                                                int width,
                                                float& y,
                                                VerticalPositioning& positioning,
                                                float scale,
                                                bool last,
                                                bool counterBaselineTranslation,
                                                bool alphaMask) const
{
    DrawResult result;

    if (glyphs_.empty()) {
        log_.warn("No glyphs in ParagraphShape.");
        return result;
    }

    const float align = evaluateAlign(glyphs_[0].format.align, glyphs_[0].direction);
    auto bearingX = glyphs_[0].bearingX * scale;

    Vector2f caret{0.0f, y};
    for (auto& lineSpan : lineSpans_) {
        result.journal.startLine(caret.y);

        startCaret(lineSpan, caret.y, positioning, scale);

        // Resolve justification
        JustifyParams justifyParams = {
                (width > 0.0 ? width : lineSpan.lineWidth) / scale,
                ctx.config.justifyAmbiguous,
                ctx.config.limitJustifySpaceWidth
            };

        auto justification = justify(lineSpan, justifyParams);
        if (!justification.success) {
            positioning = VerticalPositioning::PREV_BASELINE;
            continue;
        }

        auto spaceCoef = justification.spaceCoef;
        auto nonSpaceCoef = justification.nonSpaceCoef;
        auto lineWidth = justification.lineWidth * scale;

        auto leftLimit = left + align * (width - lineWidth);
        auto rightLimit = left + width - (1.0f - align) * (width - lineWidth);

        bool firstLine = &lineSpan == &lineSpans_.front();

        // first line of the paragraph
        if (firstLine) {
            result.firstAscender = maxAscender(lineSpan, scale);
            result.firstDescender = maxDescender(lineSpan, scale);
            result.firstLineHeight = maxLineHeight(lineSpan, scale);
            result.firstLineDefaultHeight = maxLineDefaultHeight(lineSpan, scale);
            result.firstLineBearing = maxBearing(lineSpan, scale);
            result.leftFirst = leftLimit;
        }
        // last line of the paragraph
        if (&lineSpan == &lineSpans_.back()) {
            result.lastlineDescender = maxDescender(lineSpan, scale);
        }

        bool lineRtl = lineSpan.baseDir == TextDirection::RIGHT_TO_LEFT;
        caret.x = (lineRtl ? rightLimit : leftLimit)/* * scale*/;

        if (firstLine) {
            caret.x += glyphs_[0].format.paragraphIndent * scale;
        }

        // Draw the line
        for (const auto& visualRun : lineSpan.visualRuns) {
            bool runRtl = visualRun.dir == TextDirection::RIGHT_TO_LEFT && ctx.config.enableRtl;
            auto direction = runRtl ? -1 : 1;
            auto runWidth = visualRun.width * scale;

            bool firstRun = &visualRun == &lineSpan.visualRuns.front();

            // jump to the beginning of the run
            if (runRtl != lineRtl)
                caret.x += -1 * direction * runWidth;

            for (int j = static_cast<int>(visualRun.start); j < static_cast<int>(visualRun.end); ++j) {
                auto unscaledGlyphShape = glyphs_[j];
                auto scaledGlyphShape = glyphs_[j].scaledGlyphShape(scale);
                bool fixedHorizontalAdvance = false;

                if (unscaledGlyphShape.character == 0x2028) { // Blank line
                    positioning = VerticalPositioning::PREV_BASELINE;
                    continue;
                }

                auto tabStop = isTabStop(unscaledGlyphShape.character);

                if (j == visualRun.start && !firstLine && firstRun && !tabStop) {
                    auto x = caret.x;
                    auto nextlineOffset = newlineOffset(glyphs_[j].format.tabStops);
                    if (nextlineOffset.has_value()) {
                        caret.x = nextlineOffset.value() * scale;
                    }
                }

                if (tabStop) {
                    auto tabstopIt = unscaledGlyphShape.format.tabStops.upper_bound(caret.x / scale);
                    if (tabstopIt != end(unscaledGlyphShape.format.tabStops)) {
                        caret.x = *tabstopIt * scale;
                        fixedHorizontalAdvance = true;
                    }
                }

                const FaceId &faceID = unscaledGlyphShape.format.faceId;
                const FaceTable::Item* faceItem = fontManager_.facesTable().getFaceItem(faceID);
                if (!faceItem) {
                    log_.warn("Line drawing error: Missing font face \"{}\"", faceID);
                    continue;
                }

                auto face = faceItem->face;

                auto glyphScale = 1.0f;
                auto desiredSize = face->isScalable() ? unscaledGlyphShape.format.size : scaledGlyphShape.size;

                auto setSizeRes = face->setSize(desiredSize);
                if (setSizeRes && !face->isScalable()) {
                    glyphScale = scaledGlyphShape.ascender / (float)setSizeRes.value();
                }

                auto offset = Vector2f{
                    static_cast<float>(caret.x - floor(caret.x)),
                    static_cast<float>(caret.y - floor(caret.y)),
                };

                auto glyph =
                    face->acquireGlyph(unscaledGlyphShape.codepoint, offset, {scale,glyphScale}, true,
                                       ctx.config.internalDisableHinting);
                if (!glyph) {
                    continue;
                }

                if (ctx.config.allowTrueTypeKerning && j > lineSpan.start) {
                    if (!face->hasOpenTypeFeature(otf::feature::KERN) || ctx.config.forceTrueTypeKerning) {
                        caret.x += direction * kern(face, j, scale);
                    } else {
                        // OpenType kerning is already applied as a part shaping process.
                        // Normally, the two kerning apporaches should be mutually exclusive,
                        // i.e. there's either a dedicated 'kern' table xor there's OpenType
                        // 'kern' feature. However fonts exist that contain both.
                        //
                        // Unless forced by configuration setting `forceTrueTypeKerning`, the
                        // OpenType kerning is prefered.
                    }
                }

                if (j == lineSpan.start && ctx.config.disregardFirstBearing) {
                    caret.x -= direction * glyph->bitmapBearing.x;
                }

                Vector2f coord = {caret.x, caret.y};
                coord.x += glyph->bitmapBearing.x;
                coord.y -= glyph->bitmapBearing.y;

                spacing hAdvance = scaledGlyphShape.horizontalAdvance * (1 - (int)fixedHorizontalAdvance);
                if (runRtl) {
                    coord.x -= hAdvance + scaledGlyphShape.letterSpacing;
                }

                caret.x += direction * (isWhitespace(unscaledGlyphShape.character) ? spaceCoef : nonSpaceCoef) *
                           (hAdvance + scaledGlyphShape.letterSpacing);
                auto fracOffset = glyph->rsb_delta - glyph->lsb_delta;
                caret.x += fracOffset * scale;

                glyph->setDestination({static_cast<int>(floor(coord.x)), static_cast<int>(floor(coord.y))},
                                      counterBaselineTranslation ? 0 : maxAscender(lineSpan, scale));
                glyph->setColor(alphaMask ? ~Pixel32() : unscaledGlyphShape.color);

                if (unscaledGlyphShape.format.decoration != Decoration::NONE) {
                    result.journal.addDecoration(
                        {{static_cast<int>(floor(coord.x)), static_cast<int>(floor(caret.y))},
                         {static_cast<int>(round(coord.x)) + glyph->bitmapWidth(), static_cast<int>(floor(caret.y))},
                         unscaledGlyphShape.format.decoration,
                         unscaledGlyphShape.color,
                         face}, scale);
                }

                if (!tabStop) {
                    result.journal.addGlyph(std::move(glyph));
                }
            }

            // jump again to the beginning of the run and continue in original direction
            if (runRtl != lineRtl)
                caret.x += -1 * direction * runWidth;
        }

        positioning = VerticalPositioning::PREV_BASELINE;

        // updates max line width in the paragraph
        if (width) {
            // for fixed width use the provided value
            result.maxLineWidth = static_cast<float>(width);
        } else {
            // TODO what if RTL??
            auto currentLineWidth = std::ceil(std::max(caret.x - leftLimit, lineWidth));
            result.maxLineWidth = std::max(result.maxLineWidth, currentLineWidth);
        }

        // updates the leftmost coords within paragraph
        result.leftmost = std::min(result.leftmost, !lineRtl ? leftLimit : lineSpan.lineWidth - leftLimit);
    }
    y = caret.y + ((last ? 0 : 1) * (glyphs_[0].format.paragraphSpacing * scale));

    return result;
}

HorizontalAlign ParagraphShape::firstLineHorizontalAlignment() const
{
    if (glyphs_.empty()) {
        return HorizontalAlign::LEFT;
    }

    return glyphs_[0].format.align;
}

bool ParagraphShape::hasExplicitLineHeight() const
{
    if (glyphs_.empty()) {
        return false;
    }

    return glyphs_[0].format.lineHeight != 0;
}

const LineSpans& ParagraphShape::lineSpans() const
{
    return lineSpans_;
}

std::size_t ParagraphShape::linesCount() const
{
    return lineSpans_.size();
}

const GlyphShape &ParagraphShape::glyph(std::size_t index) const
{
    return glyphs_[index];
}

const std::vector<GlyphShape> &ParagraphShape::glyphs() const
{
    return glyphs_;
}

void ParagraphShape::startCaret(const LineSpan& lineSpan, float& y, VerticalPositioning& positioning, float scale) const
{
    float yShift = 0;
    switch (positioning) {
        case VerticalPositioning::BASELINE:

            break;
        case VerticalPositioning::PREV_BASELINE:
            yShift = std::round(maxLineHeight(lineSpan, scale));
            break;
        case VerticalPositioning::TOP_BOUND:
            yShift = maxAscender(lineSpan, scale);
            break;
    }

    /*
    // this is how to find "default" baseline
    if (true && positioning != VerticalPositioning::PREV_BASELINE) {
        auto gap = glyphs_[0].lineHeight - (yShift - maxDescender(lineSpan));
        yShift += gap * 0.5;
    }
    */

    y += yShift;
    positioning = VerticalPositioning::BASELINE;
}

spacing ParagraphShape::evalLineHeight(const uint32_t codepoint, const ImmediateFormat& fmt, const FacePtr face) const
{
    spacing lh = fmt.lineHeight;

    if (lh == 0) {

        // implicit line height
        lh = lineHeightFromFace(face);

        if (lh == fmt.size) {
            // line height is typically expected to be larger than fmt.size
            // setting it to span of ascender and descender is on of the options
            // we can do with it to achieve extecpted results

            spacing ascender;
            spacing descender;

            if (face->isScalable()) {
                ascender = face->scaleFontUnits(face->getFtFace()->ascender, true);
                descender = face->scaleFontUnits(face->getFtFace()->descender, true);
            } else {
                ascender = FreetypeHandle::from26_6fixed(face->getFtFace()->size->metrics.ascender);
                descender = FreetypeHandle::from26_6fixed(face->getFtFace()->size->metrics.descender);
            }

            lh = ascender - descender;

            /*
            // line height is typically expected to be larger than fmt.size
            // adjusting by descender value is just because we currently don't
            // know any better
            auto descender = 0.0f;

            if (face->isScalable()) {
                descender = face->scaleFontUnits(face->getFtFace()->descender, true);
            } else {
                descender = FreetypeHandle::from26_6fixed(face->getFtFace()->size->metrics.descender);
            }

            lh -= descender;
            */
        }
    }

    // limit the height
    if (fmt.minLineHeight > 0) {
        lh = std::max(lh, fmt.minLineHeight);
    }
    if (fmt.maxLineHeight > 0) {
        lh = std::min(lh, fmt.maxLineHeight);
    }

    return lh;
}

ParagraphShape::JustifyResult ParagraphShape::justify(const LineSpan& lineSpan, JustifyParams params) const
{
    float spaceCoef = 1.0f;
    float nonSpaceCoef = 1.0f;
    auto startIdx = static_cast<std::size_t>(lineSpan.start);

    if (startIdx >= glyphs_.size()) {
        log_.warn("Line drawing error: Line length equals zero.");
        return {0.0f, 0.0f, 0.0f, false};
    }

    bool shouldJustify = glyphs_[startIdx].format.align == HorizontalAlign::JUSTIFY;
    shouldJustify =
        shouldJustify && (lineSpan.justifiable == LineSpan::Justifiable::POSITIVE ||
                          (lineSpan.justifiable == LineSpan::Justifiable::DOCUMENT && params.justifyAmbiguous));
    if (shouldJustify) {
        int spaces = 0;
        int nonSpaces = 0;
        spacing spaceWidth = 0;
        spacing nonSpaceWidth = 0;
        for (int j = static_cast<int>(lineSpan.start); j < static_cast<int>(lineSpan.end); ++j) {
            if (isWhitespace(glyphs_[j].character)) {
                spaces++;
                spaceWidth += glyphs_[j].horizontalAdvance + glyphs_[j].format.letterSpacing;
            } else {
                nonSpaces++;
                nonSpaceWidth += glyphs_[j].horizontalAdvance + glyphs_[j].format.letterSpacing;
            }
        }

        spacing spaceDebt = params.width - lineSpan.lineWidth;
        spaceCoef += spaceDebt / spaceWidth;
        if ((spaceCoef > MAX_SPACE_COEF && params.limitJustifySpaceWidth) || !std::isfinite(spaceCoef)) {
            spaceCoef = MAX_SPACE_COEF;
            nonSpaceCoef += (spaceDebt - (spaceCoef - 1.0f) * spaceWidth) / nonSpaceWidth;
        }
        // TODO Sketch assumably also contains a MAX_NONSPACE_COEF so that spaces start expanding again, but that is not
        // very common.
    }

    return {spaceCoef, nonSpaceCoef, lineSpan.lineWidth, true};
}

float ParagraphShape::kern(const FacePtr face, const int idx, float scale) const
{
    float kernValue = 0.0;
    auto fontHasKerning = FT_HAS_KERNING(face->getFtFace());
    auto glyphHasKerning = glyphs_[idx].format.kerning;

    if (fontHasKerning && glyphHasKerning) {
        FT_Vector kerning;
        auto prev = glyphs_[idx - 1].codepoint;
        auto curr = glyphs_[idx].codepoint;
        FreetypeHandle::error = FT_Get_Kerning(face->getFtFace(), prev, curr, FT_KERNING_DEFAULT, &kerning);
        FreetypeHandle::checkOk(__func__);
        kernValue = scale * FreetypeHandle::from26_6fixed(kerning.x); // grid-fitted kerning distances, see FT_Kerning_Mode
    }

    return kernValue;
}

spacing ParagraphShape::maxDescender(const LineSpan& lineSpan, float scale) const
{
    spacing descender = 0;

    for (auto i = lineSpan.start; i < lineSpan.end; ++i) {
        descender = std::min(glyphs_[i].descender * scale, descender);
    }

    return descender;
}

spacing ParagraphShape::maxAscender(const LineSpan& lineSpan, float scale) const
{
    spacing ascender = 0;

    for (auto i = lineSpan.start; i < lineSpan.end; ++i) {
        ascender = std::max(glyphs_[i].ascender * scale, ascender);
    }

    return ascender;
}

spacing ParagraphShape::maxLineHeight(const LineSpan& lineSpan, float scale) const
{
    spacing height = 0;

    for (auto i = lineSpan.start; i < lineSpan.end; ++i) {
        height = std::max(glyphs_[i].lineHeight * scale, height);
    }

    return height;
}

spacing ParagraphShape::maxLineDefaultHeight(const LineSpan& lineSpan, float scale) const
{
    spacing defaultLineHeight = 0;

    for (auto i = lineSpan.start; i < lineSpan.end; ++i) {
        defaultLineHeight = std::max(glyphs_[i].defaultLineHeight * scale, defaultLineHeight);
    }

    return defaultLineHeight;
}

spacing ParagraphShape::maxBearing(const LineSpan& lineSpan, float scale) const
{
    spacing bearing = 0;

    for (auto i = lineSpan.start; i < lineSpan.end; ++i) {
        bearing = std::max(glyphs_[i].bearingY * scale, bearing);
    }

    return bearing;
}

std::vector<hb_feature_t> ParagraphShape::setupFeatures(const ImmediateFormat& format) const
{
    typedef std::pair<const char*, std::uint32_t> featureItem;

    auto liga = format.ligatures == GlyphFormat::Ligatures::STANDARD || format.ligatures == GlyphFormat::Ligatures::ALL;
    auto alt =
        format.ligatures == GlyphFormat::Ligatures::ALTERNATIVE || format.ligatures == GlyphFormat::Ligatures::ALL;

    // those **typically** correspond to OpenType features
    auto tags = std::vector<featureItem>{
        featureItem(otf::feature::LIGA, liga),           // standard ligatures
        featureItem(otf::feature::DLIG, alt),            // discretionary ligatures
        featureItem(otf::feature::HLIG, alt),            // historical ligatures
        featureItem(otf::feature::CLIG, alt),            // contextual ligatures / alternates
        featureItem(otf::feature::KERN, format.kerning), // modern kerning feature
    };

    for (const auto& userFeature : format.features) {

        tags.emplace_back(featureItem(userFeature.tag.c_str(), userFeature.value));
    }

    std::vector<hb_feature_t> hbFeatures;
    for (const auto& tag : tags) {
        hb_feature_t ftr;
        if (hb_feature_from_string(tag.first, -1, &ftr)) {
            ftr.value = tag.second;
            hbFeatures.emplace_back(std::move(ftr));
        }
    }

    return hbFeatures;
}

bool ParagraphShape::validateUserFeatures(const FacePtr face, const TypeFeatures& features) const
{
    bool ok = true;
    for (const auto& userFeature : features) {
        const auto& tag = userFeature.tag;
        if (!face->hasOpenTypeFeature(userFeature.tag)) {
            log_.warn("missing OpenType feature: {}", tag);
            ok = false;
        }
    }
    return ok;
}

void ParagraphShape::shapeSequence(const Sequence& seq,
                                   const FormattedParagraph& paragraph,
                                   const FaceTable& faces,
                                   ShapeResult& result)
{
    auto faceItem = faces.getFaceItem(seq.format.faceId);
    if (!faceItem) {
        bool inserted = false;
        std::tie(std::ignore, inserted) = reportedFaces_.insert({seq.format.faceId, ReportedFontReason::NO_DATA});
        if (inserted) {
            log_.warn("Paragraph shaping error: Reported face item '{}'", seq.format.faceId);
        }
        return;
    }

    auto face = faceItem->face;
    if (!face->getFtFace()) {
        reportedFaces_.insert({seq.format.faceId, ReportedFontReason::LOAD_FAILED});
        log_.warn("Paragraph shaping error: Missing font face '{}'", seq.format.faceId);
        return;
    } else {
        result.insertFontItem(seq.format.faceId, faceItem->fallback);
    }

    if (face->getPostScriptName() != seq.format.faceId) {
        reportedFaces_.insert({seq.format.faceId, ReportedFontReason::POSTSCRIPTNAME_MISMATCH});
    }

    auto desiredSize = seq.format.size;
    auto actualSizeResult = face->setSize(desiredSize);
    auto resizeFactor = 1.0f;
    if (actualSizeResult && actualSizeResult.value() != desiredSize) {
        resizeFactor = (float)desiredSize / (float)actualSizeResult.value();
    }

    hb_buffer_t* hbBuffer = hb_buffer_create();
    hb_buffer_add_utf32(hbBuffer, &paragraph.text_[0], static_cast<int>(paragraph.text_.size()), seq.start, seq.len);
    hb_buffer_guess_segment_properties(hbBuffer);
    auto rtl = hb_buffer_get_direction(hbBuffer) == HB_DIRECTION_RTL;

    validateUserFeatures(face, paragraph.format_[seq.start].features);
    auto hbFeatures = setupFeatures(paragraph.format_[seq.start]);

    hb_shape(face->getHbFont(), hbBuffer, hbFeatures.data(), static_cast<unsigned int>(hbFeatures.size()));

    auto missingGlyph = false;

    GlyphShape glyph;
    auto hbLen = hb_buffer_get_length(hbBuffer);
    const hb_glyph_position_t* hbPos = hb_buffer_get_glyph_positions(hbBuffer, nullptr);
    const hb_glyph_info_t* hbInfo = hb_buffer_get_glyph_infos(hbBuffer, nullptr);
    for (unsigned i = 0; i < hbLen; ++i) {
        int k = rtl ? hbLen - i - 1 : i;
        int p = hbInfo[k].cluster;
        const auto& fmt = paragraph.format_[p];

        // If HB does not evaluate emoji modifiers and ZWJ sequences correctly,
        // force skip those pseudo-glyphs. Known to happen with Apple Color Emoji.
        if (isSkipGlyph(paragraph.text_[i]))
            continue;

        glyph.format = fmt;
        glyph.codepoint = hbInfo[k].codepoint;
        glyph.color = fmt.color;

        if (lineStarts_)
            glyph.lineStart = std::optional<bool>(lineStarts_->at(p));

        if (hbPos[k].x_advance != 0) {
            glyph.horizontalAdvance =
                FreetypeHandle::from26_6fixed(hbPos[k].x_advance); // HB has already scaled FT values
        } else {
            glyph.horizontalAdvance = FreetypeHandle::from16_16fixed(face->getGlyphAdvance(glyph.codepoint));
        }

        glyph.direction = rtl ? TextDirection::RIGHT_TO_LEFT : TextDirection::LEFT_TO_RIGHT;
        glyph.character = paragraph.text_[p];
        glyph.lineHeight = evalLineHeight(glyph.codepoint, fmt, face);
        glyph.defaultLineHeight = lineHeightFromFace(face);

        if (face->isScalable()) {
            glyph.ascender = face->scaleFontUnits(face->getFtFace()->ascender, true);
            glyph.descender = face->scaleFontUnits(face->getFtFace()->descender, true);
        } else {
            // the values in size->metrics are rounded for historical reasons and therefore less desirable
            // however "global" values in font units are not present in bitmap fonts
            // see docs for FT_Size_Metrics and FT_FaceRec for more info
            glyph.ascender = FreetypeHandle::from26_6fixed(face->getFtFace()->size->metrics.ascender);
            glyph.descender = FreetypeHandle::from26_6fixed(face->getFtFace()->size->metrics.descender);
        }

        if (glyph.codepoint == 0 && !LineBreaker::isSoftBreak(glyph.character)) {
            missingGlyph = true;
        }

        if (!face->isScalable()) {
            glyph.applyResizeFactor(resizeFactor);
        }

        auto a = FreetypeHandle::from26_6fixed(face->getFtFace()->size->metrics.ascender);
        auto d = FreetypeHandle::from26_6fixed(face->getFtFace()->size->metrics.descender);

        if (loadBearing_) {
            auto loadedGlyph = face->acquireGlyph(glyph.codepoint, {}, {1.0f,1.0f}, false, true);
            glyph.bearingX = loadedGlyph->metricsBearingX;
            glyph.bearingY = loadedGlyph->metricsBearingY;
        } else {
            glyph.bearingX = glyph.bearingY = 0.0f;
        }

        glyphs_.push_back(glyph);
    }

    hb_buffer_destroy(hbBuffer);

    if (missingGlyph) {
        reportedFaces_.insert({seq.format.faceId, ReportedFontReason::MISSING_GLYPH});
    }
}

void ParagraphShape::ShapeResult::insertFontItem(const std::string& faceID, bool fallback)
{
    fallbacks.insert({faceID, fallback});
}

} // namespace priv
} // namespace textify
