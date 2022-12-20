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


ParagraphShape::ParagraphShape(const utils::Log& log, const FaceTable &faceTable) :
    log_(log),
    faceTable_(faceTable)
{
}

void ParagraphShape::initialize(const GlyphShapes &glyphs,
                                const LineSpans &lineSpans) {
    shapingPhaseOutput_.glyphs_ = glyphs;
    shapingPhaseOutput_.lineSpans_ = lineSpans;
}

ParagraphShape::ShapeResult ParagraphShape::shape(const FormattedParagraph& paragraph, float width, bool loadGlyphsBearings)
{
    if (paragraph.text_.size() != paragraph.format_.size()) {
        log_.warn("[Textify / ParagraphShape::shape] Paragraph shaping error: Text format incorrectly expanded.");
        return false;
    }

    // Prepare Unicode line break opportunities
    const LineBreaker::LineStarts lineStartsOpt = LineBreaker::analyzeBreaks(log_,  paragraph.text_);

    ShapeResult result(false);
    const int textLen = static_cast<int>(paragraph.text_.size());
    int pos = 0;

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

        shapeSequence(seq, paragraph, faceTable_, lineStartsOpt, loadGlyphsBearings, result);
    }

    LineBreaker breaker {log_, shapingPhaseOutput_.glyphs_, paragraph.visualRuns_, paragraph.baseDirection_};
    breaker.breakLines(static_cast<int>(std::floor(width)));
    const LineSpans &lineSpans = breaker.getLines();
    if (lineSpans.empty()) {
        log_.warn("[Textify / ParagraphShape::shape] Paragraph breaking error: No lines present.");
        return result;
    }

    shapingPhaseOutput_.lineSpans_ = breaker.moveLines();

    result.success = !shapingPhaseOutput_.glyphs_.empty();
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

    if (shapingPhaseOutput_.glyphs_.empty()) {
        log_.warn("[Textify / ParagraphShape::draw] No glyphs in ParagraphShape.");
        return result;
    }

    const float align = evaluateAlign(shapingPhaseOutput_.glyphs_[0].format.align, shapingPhaseOutput_.glyphs_[0].direction);
    const spacing bearingX = shapingPhaseOutput_.glyphs_[0].bearingX * scale;

    Vector2f caret{0.0f, y};
    for (const LineSpan &lineSpan : shapingPhaseOutput_.lineSpans_) {
        result.journal.startLine(caret.y);

        startCaret(lineSpan, caret.y, positioning, scale);

        // Resolve justification
        const JustifyParams justifyParams {
                (width > 0.0 ? width : lineSpan.lineWidth) / scale,
                ctx.config.justifyAmbiguous,
                ctx.config.limitJustifySpaceWidth
            };

        const JustifyResult justification = justify(lineSpan, justifyParams);
        if (!justification.success) {
            positioning = VerticalPositioning::PREV_BASELINE;
            continue;
        }

        const float spaceCoef = justification.spaceCoef;
        const float nonSpaceCoef = justification.nonSpaceCoef;
        const float lineWidth = justification.lineWidth * scale;

        const int leftLimit = left + align * (width - lineWidth);
        const int rightLimit = left + width - (1.0f - align) * (width - lineWidth);

        const bool isFirstLine = &lineSpan == &shapingPhaseOutput_.lineSpans_.front();

        // first line of the paragraph
        if (isFirstLine) {
            result.firstAscender = maxAscender(lineSpan, scale);
            result.firstDescender = maxDescender(lineSpan, scale);
            result.firstLineHeight = maxLineHeight(lineSpan, scale);
            result.firstLineDefaultHeight = maxLineDefaultHeight(lineSpan, scale);
            result.firstLineBearing = maxBearing(lineSpan, scale);
            result.leftFirst = leftLimit;
        }
        // last line of the paragraph
        if (&lineSpan == &shapingPhaseOutput_.lineSpans_.back()) {
            result.lastlineDescender = maxDescender(lineSpan, scale);
        }

        const bool lineRtl = lineSpan.baseDir == TextDirection::RIGHT_TO_LEFT;
        caret.x = (lineRtl ? rightLimit : leftLimit)/* * scale*/;

        if (isFirstLine) {
            caret.x += shapingPhaseOutput_.glyphs_[0].format.paragraphIndent * scale;
        }

        // Draw the line
        for (const VisualRun &visualRun : lineSpan.visualRuns) {
            const bool runRtl = visualRun.dir == TextDirection::RIGHT_TO_LEFT && ctx.config.enableRtl;
            const float direction = runRtl ? -1.0f : 1.0f;
            const float runWidth = visualRun.width * scale;

            bool firstRun = &visualRun == &lineSpan.visualRuns.front();

            // jump to the beginning of the run
            if (runRtl != lineRtl)
                caret.x += -1 * direction * runWidth;

            for (int j = static_cast<int>(visualRun.start); j < static_cast<int>(visualRun.end); ++j) {
                const GlyphShape &unscaledGlyphShape = shapingPhaseOutput_.glyphs_[j];
                const GlyphShape::Scalable &scaledGlyphShape = shapingPhaseOutput_.glyphs_[j].scaledGlyphShape(scale);
                bool fixedHorizontalAdvance = false;

                if (unscaledGlyphShape.character == 0x2028) { // Blank line
                    positioning = VerticalPositioning::PREV_BASELINE;
                    continue;
                }

                const bool tabStop = isTabStop(unscaledGlyphShape.character);

                if (j == visualRun.start && !isFirstLine && firstRun && !tabStop) {
                    auto x = caret.x;
                    auto nextlineOffset = newlineOffset(shapingPhaseOutput_.glyphs_[j].format.tabStops);
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
                const FaceTable::Item* faceItem = faceTable_.getFaceItem(faceID);
                if (!faceItem) {
                    log_.warn("[Textify / ParagraphShape::draw] Line drawing error: Missing font face \"{}\"", faceID);
                    continue;
                }

                FacePtr face = faceItem->face;

                float glyphScale = 1.0f;
                const font_size desiredSize = face->isScalable() ? unscaledGlyphShape.format.size : scaledGlyphShape.size;

                const Result<font_size,bool> setSizeRes = face->setSize(desiredSize);
                if (setSizeRes && !face->isScalable()) {
                    glyphScale = scaledGlyphShape.ascender / (float)setSizeRes.value();
                }

                const Vector2f offset {
                    static_cast<float>(caret.x - floor(caret.x)),
                    static_cast<float>(caret.y - floor(caret.y)),
                };

                GlyphPtr glyph = face->acquireGlyph(unscaledGlyphShape.codepoint, offset, {scale,glyphScale}, true, ctx.config.internalDisableHinting);
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
                const float fracOffset = glyph->rsb_delta - glyph->lsb_delta;
                caret.x += fracOffset * scale;

                glyph->setDestination({static_cast<int>(floor(coord.x)), static_cast<int>(floor(coord.y))},
                                      counterBaselineTranslation ? 0 : maxAscender(lineSpan, scale));
                glyph->setColor(alphaMask ? ~Pixel32() : unscaledGlyphShape.format.color);

                if (unscaledGlyphShape.format.decoration != Decoration::NONE) {
                    result.journal.addDecoration(
                        {{static_cast<int>(floor(coord.x)), static_cast<int>(floor(caret.y))},
                         {static_cast<int>(round(coord.x)) + glyph->bitmapWidth(), static_cast<int>(floor(caret.y))},
                         unscaledGlyphShape.format.decoration,
                         unscaledGlyphShape.format.color,
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
            // TODO: what if RTL??
            auto currentLineWidth = std::ceil(std::max(caret.x - leftLimit, lineWidth));
            result.maxLineWidth = std::max(result.maxLineWidth, currentLineWidth);
        }

        // updates the leftmost coords within paragraph
        result.leftmost = std::min(result.leftmost, !lineRtl ? leftLimit : lineSpan.lineWidth - leftLimit);
    }
    y = caret.y + ((last ? 0 : 1) * (shapingPhaseOutput_.glyphs_[0].format.paragraphSpacing * scale));

    return result;
}

HorizontalAlign ParagraphShape::firstLineHorizontalAlignment() const
{
    if (shapingPhaseOutput_.glyphs_.empty()) {
        return HorizontalAlign::LEFT;
    }

    return shapingPhaseOutput_.glyphs_[0].format.align;
}

bool ParagraphShape::hasExplicitLineHeight() const
{
    if (shapingPhaseOutput_.glyphs_.empty()) {
        return false;
    }

    return shapingPhaseOutput_.glyphs_[0].format.lineHeight != 0;
}

const GlyphShape &ParagraphShape::glyph(std::size_t index) const
{
    return shapingPhaseOutput_.glyphs_[index];
}

const std::vector<GlyphShape> &ParagraphShape::glyphs() const
{
    return shapingPhaseOutput_.glyphs_;
}

const LineSpans& ParagraphShape::lineSpans() const
{
    return shapingPhaseOutput_.lineSpans_;
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

            spacing ascender = 0.0f;
            spacing descender = 0.0f;

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

ParagraphShape::JustifyResult ParagraphShape::justify(const LineSpan& lineSpan, const JustifyParams &params) const
{
    float spaceCoef = 1.0f;
    float nonSpaceCoef = 1.0f;
    const size_t startIdx = static_cast<std::size_t>(lineSpan.start);

    if (startIdx >= shapingPhaseOutput_.glyphs_.size()) {
        log_.warn("[Textify / ParagraphShape::justify] Line drawing error: Line length equals zero.");
        return {0.0f, 0.0f, 0.0f, false};
    }

    const bool shouldJustify = shapingPhaseOutput_.glyphs_[startIdx].format.align == HorizontalAlign::JUSTIFY &&
         (lineSpan.justifiable == LineSpan::Justifiable::POSITIVE ||
         (lineSpan.justifiable == LineSpan::Justifiable::DOCUMENT && params.justifyAmbiguous));

    if (shouldJustify) {
        int spaces = 0;
        int nonSpaces = 0;
        spacing spaceWidth = 0;
        spacing nonSpaceWidth = 0;
        for (int j = static_cast<int>(lineSpan.start); j < static_cast<int>(lineSpan.end); ++j) {
            if (isWhitespace(shapingPhaseOutput_.glyphs_[j].character)) {
                spaces++;
                spaceWidth += shapingPhaseOutput_.glyphs_[j].horizontalAdvance + shapingPhaseOutput_.glyphs_[j].format.letterSpacing;
            } else {
                nonSpaces++;
                nonSpaceWidth += shapingPhaseOutput_.glyphs_[j].horizontalAdvance + shapingPhaseOutput_.glyphs_[j].format.letterSpacing;
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
    const bool fontHasKerning = FT_HAS_KERNING(face->getFtFace());
    const bool glyphHasKerning = shapingPhaseOutput_.glyphs_[idx].format.kerning;

    if (fontHasKerning && glyphHasKerning) {
        FT_Vector kerning;
        const uint32_t prev = shapingPhaseOutput_.glyphs_[idx - 1].codepoint;
        const uint32_t curr = shapingPhaseOutput_.glyphs_[idx].codepoint;
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
        descender = std::min(shapingPhaseOutput_.glyphs_[i].descender * scale, descender);
    }

    return descender;
}

spacing ParagraphShape::maxAscender(const LineSpan& lineSpan, float scale) const
{
    spacing ascender = 0;

    for (auto i = lineSpan.start; i < lineSpan.end; ++i) {
        ascender = std::max(shapingPhaseOutput_.glyphs_[i].ascender * scale, ascender);
    }

    return ascender;
}

spacing ParagraphShape::maxLineHeight(const LineSpan& lineSpan, float scale) const
{
    spacing height = 0;

    for (auto i = lineSpan.start; i < lineSpan.end; ++i) {
        height = std::max(shapingPhaseOutput_.glyphs_[i].lineHeight * scale, height);
    }

    return height;
}

spacing ParagraphShape::maxLineDefaultHeight(const LineSpan& lineSpan, float scale) const
{
    spacing defaultLineHeight = 0;

    for (auto i = lineSpan.start; i < lineSpan.end; ++i) {
        defaultLineHeight = std::max(shapingPhaseOutput_.glyphs_[i].defaultLineHeight * scale, defaultLineHeight);
    }

    return defaultLineHeight;
}

spacing ParagraphShape::maxBearing(const LineSpan& lineSpan, float scale) const
{
    spacing bearing = 0;

    for (auto i = lineSpan.start; i < lineSpan.end; ++i) {
        bearing = std::max(shapingPhaseOutput_.glyphs_[i].bearingY * scale, bearing);
    }

    return bearing;
}

std::vector<hb_feature_t> ParagraphShape::setupFeatures(const ImmediateFormat& format) const
{
    typedef std::pair<const char*, std::uint32_t> featureItem;

    auto liga = format.ligatures == GlyphFormat::Ligatures::STANDARD || format.ligatures == GlyphFormat::Ligatures::ALL;
    auto alt = format.ligatures == GlyphFormat::Ligatures::ALTERNATIVE || format.ligatures == GlyphFormat::Ligatures::ALL;

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
            log_.warn("[Textify / ParagraphShape::validateUserFeatures] Missing OpenType feature: {}", tag);
            ok = false;
        }
    }
    return ok;
}

void ParagraphShape::shapeSequence(const Sequence& seq,
                                   const FormattedParagraph& paragraph,
                                   const FaceTable& faces,
                                   const LineBreaker::LineStarts &lineStartsOpt,
                                   bool loadGlyphsBearings,
                                   ShapeResult& result)
{
    const FaceTable::Item* faceItem = faces.getFaceItem(seq.format.faceId);
    if (!faceItem) {
        bool inserted = false;
        std::tie(std::ignore, inserted) = reportedFaces_.insert({seq.format.faceId, ReportedFontReason::NO_DATA});
        if (inserted) {
            log_.warn("[Textify / ParagraphShape::shapeSequence] Paragraph shaping error: Reported face item '{}'", seq.format.faceId);
        }
        return;
    }

    const FacePtr face = faceItem->face;
    if (!face->getFtFace()) {
        reportedFaces_.insert({seq.format.faceId, ReportedFontReason::LOAD_FAILED});
        log_.warn("[Textify / ParagraphShape::shapeSequence] Paragraph shaping error: Missing font face '{}'", seq.format.faceId);
        return;
    } else {
        result.insertFontItem(seq.format.faceId, faceItem->fallback);
    }

    if (face->getPostScriptName() != seq.format.faceId) {
        reportedFaces_.insert({seq.format.faceId, ReportedFontReason::POSTSCRIPTNAME_MISMATCH});
    }

    auto desiredSize = seq.format.size;
    auto actualSizeResult = face->setSize(desiredSize);
    float resizeFactor = 1.0f;
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

    bool isGlyphMissing = false;

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
        if (isSkipGlyph(paragraph.text_[i])) {
            log_.info("[Textify / ParagraphShape::shapeSequence] Skipping unrecognized glyph '{}'", paragraph.text_[i]);
            continue;
        }

        glyph.format = fmt;
        glyph.codepoint = hbInfo[k].codepoint;

        if (lineStartsOpt)
            glyph.lineStart = std::optional<bool>(lineStartsOpt->at(p));

        if (hbPos[k].x_advance != 0) {
            glyph.horizontalAdvance = FreetypeHandle::from26_6fixed(hbPos[k].x_advance); // HB has already scaled FT values
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
            isGlyphMissing = true;
        }

        if (!face->isScalable()) {
            glyph.applyResizeFactor(resizeFactor);
        }

        if (loadGlyphsBearings) {
            const GlyphPtr loadedGlyph = face->acquireGlyph(glyph.codepoint, {}, {1.0f,1.0f}, false, true);
            glyph.bearingX = loadedGlyph->metricsBearingX;
            glyph.bearingY = loadedGlyph->metricsBearingY;
        } else {
            glyph.bearingX = glyph.bearingY = 0.0f;
        }

        shapingPhaseOutput_.glyphs_.push_back(glyph);
    }

    hb_buffer_destroy(hbBuffer);

    if (isGlyphMissing) {
        reportedFaces_.insert({seq.format.faceId, ReportedFontReason::MISSING_GLYPH});
    }
}

void ParagraphShape::ShapeResult::insertFontItem(const std::string& faceID, bool fallback)
{
    fallbacks.insert({faceID, fallback});
}

} // namespace priv
} // namespace textify
