#include "GlyphAcquisitor.h"
#include "FreetypeHandle.h"

namespace textify {

using namespace compat;

/*
int moveTo(const FT_Vector* to, void* decompositor)
{
    static_cast<Decompositor*>(decompositor)->parts.push_back(Path::Component({}, true));
    static_cast<Decompositor*>(decompositor)->currentPoint = FreetypeHandle::from26_6fixed(*to);

    return 0;
}

int lineTo(const FT_Vector* to, void* decompositor)
{
    auto& points = static_cast<Decompositor*>(decompositor)->parts.back().points;
    points.push_back(Point::createLine(FreetypeHandle::from26_6fixed(*to), 0.0f));
    static_cast<Decompositor*>(decompositor)->currentPoint = FreetypeHandle::from26_6fixed(*to);

    return 0;
}

int conicTo(const FT_Vector* control, const FT_Vector* to, void* decompositor)
{
    auto& points = static_cast<Decompositor*>(decompositor)->parts.back().points;
    const auto end = FreetypeHandle::from26_6fixed(*to);
    points.push_back(Point::createQuad(FreetypeHandle::from26_6fixed(*control), end));

    return 0;
}

int cubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* decompositor)
{
    auto& points = static_cast<Decompositor*>(decompositor)->parts.back().points;
    auto& start = static_cast<Decompositor*>(decompositor)->currentPoint;
    const auto end = FreetypeHandle::from26_6fixed(*to);
    points.push_back(Point::createCubic(start, FreetypeHandle::from26_6fixed(*control1), start));
    points.push_back(Point::createCubic(FreetypeHandle::from26_6fixed(*control2), end, end));
    start = end;

    return 0;
}
*/

GlyphAcquisitor::GlyphAcquisitor()
    /*
    : funcs_({static_cast<FT_Outline_MoveTo_Func>(&moveTo),
              static_cast<FT_Outline_LineTo_Func>(&lineTo),
              static_cast<FT_Outline_ConicTo_Func>(&conicTo),
              static_cast<FT_Outline_CubicTo_Func>(&cubicTo),
              0,
              0})
    */
{
}

void GlyphAcquisitor::setup(const Parameters& params, FT_Face ftFace)
{
    params_ = params;
    ftFace_ = ftFace;
}

GlyphPtr GlyphAcquisitor::acquire(FT_UInt codepoint, const Vector2f& offset, const ScaleParams& scale, bool render) const
{
    auto slotResult = acquireSlot(codepoint, offset, scale.vectorScale);
    if(!slotResult) {
        return nullptr;
    }
    auto glyphSlot = slotResult.value();
    /*if (!glyphSlot->glyph_index) {
        // return nullptr;
    }*/

    auto glyph = createGlyph(glyphSlot);

    bool bitmapGlyph = glyphSlot->format == FT_GLYPH_FORMAT_BITMAP; // format will change after FT_Render_Glyph

    if (params_.scalable && render) {
        FreetypeHandle::error = FT_Render_Glyph(glyphSlot, FT_RENDER_MODE_LIGHT);
        FreetypeHandle::checkOk(__func__);
    }

   auto bmpScaleFactor = scale.bitmapScale;

    if (render) {
        glyph->putBitmap(glyphSlot->bitmap);

        if (bitmapGlyph && params_.scalable)
            bmpScaleFactor *= scale.vectorScale;
        if (bmpScaleFactor != 1.f)
            glyph->scaleBitmap(bmpScaleFactor);
    }

    auto k = bmpScaleFactor;
    glyph->bitmapBearing.x = int(glyphSlot->bitmap_left * k);
    glyph->bitmapBearing.y = int(glyphSlot->bitmap_top * k);
    glyph->lsb_delta = FreetypeHandle::from26_6fixed(FT_F26Dot6(glyphSlot->lsb_delta * k));
    glyph->rsb_delta = FreetypeHandle::from26_6fixed(FT_F26Dot6(glyphSlot->rsb_delta * k));

    glyph->metricsBearingX = FreetypeHandle::from26_6fixed(FT_F26Dot6(glyphSlot->metrics.horiBearingX));
    glyph->metricsBearingY = FreetypeHandle::from26_6fixed(FT_F26Dot6(glyphSlot->metrics.horiBearingY));

    return glyph;
}

Result<FT_GlyphSlot,bool> GlyphAcquisitor::acquireSlot(FT_UInt codepoint, const Vector2f& offset, RenderScale scale) const
{
    FT_Matrix matrix = {FreetypeHandle::to16_16fixed(scale), 0, 0, FreetypeHandle::to16_16fixed(scale)};
    FT_Vector delta = {
        static_cast<FT_Pos>(FreetypeHandle::to26_6fixed(offset.x)),
       -static_cast<FT_Pos>(FreetypeHandle::to26_6fixed(offset.y))
    };

    FT_Set_Transform(ftFace_, &matrix, &delta);

    FreetypeHandle::error = FT_Load_Glyph(ftFace_, codepoint, params_.loadflags);
    auto ok = FreetypeHandle::checkOk(__func__);
    if (!ok) {
        return false;
    }

    return ftFace_->glyph;
}

/*
Path GlyphAcquisitor::acquireOutline(const FT_GlyphSlot& glyphSlot) const
{
    static const auto DUMMY = Path{{}, {0}, Matrix3f::identity, Path::Op::NO_OP, Path::Type::PATH, {0}, true, ResizingConstraints()};
    return DUMMY.clone();

    if (glyphSlot->format != FT_GLYPH_FORMAT_OUTLINE) {
        Log::instance.log(Log::TEXTIFY, Log::WARNING, "Outline acquisition: cannot load outline from bitmap glyph.");
        return DUMMY.clone();
    }

    FT_Glyph ftGlyph;
    FreetypeHandle::error = FT_Get_Glyph(ftFace_->glyph, &ftGlyph);
    FreetypeHandle::checkOk(__func__);

    auto ftOutline = reinterpret_cast<FT_OutlineGlyph>(ftGlyph)->outline; // 26.6 pixels

    Decompositor decompositor;
    FreetypeHandle::error = FT_Outline_Decompose(&ftOutline, &funcs_, &decompositor);
    FreetypeHandle::checkOk(__func__);

    // get bounds
    FT_BBox ftBBox;
    FreetypeHandle::error = FT_Outline_Get_BBox(&ftOutline, &ftBBox);
    if (!FreetypeHandle::checkOk(__func__)) {
        Log::instance.log(Log::TEXTIFY, Log::WARNING, "Outline acquisition: cannot get bounds.");
        return DUMMY.clone();
    }

    auto l = static_cast<int>(FreetypeHandle::from26_6fixed(FreetypeHandle::floor26_6(ftBBox.xMin)));
    auto t = static_cast<int>(FreetypeHandle::from26_6fixed(FreetypeHandle::ceil26_6(ftBBox.yMax)));
    auto w = static_cast<int>(
        FreetypeHandle::from26_6fixed(FreetypeHandle::ceil26_6(ftBBox.xMax) - FreetypeHandle::floor26_6(ftBBox.xMin)));
    auto h = static_cast<int>(
        FreetypeHandle::from26_6fixed(FreetypeHandle::ceil26_6(ftBBox.yMax) - FreetypeHandle::floor26_6(ftBBox.yMin)));

    auto bounds = Rectangle{l, t, w, h};

    // flip outline vertically
    auto yMin = FreetypeHandle::from26_6fixed(FreetypeHandle::floor26_6(ftBBox.yMin));
    auto m = translationMatrix({0.0f, yMin}) * scaleMatrix(1.0f, -1.0f, {0.0f, 0.5f * h});
    for (auto& part : decompositor.parts) {
        for (auto& pt : part.points) {
            pt.hardTransform(m);
        }
    }

    FT_Done_Glyph(ftGlyph);
    return Path(std::move(decompositor.parts), bounds);
}
*/

GlyphPtr GlyphAcquisitor::createGlyph(FT_GlyphSlot glyphSlot) const
{
    if (params_.isColor) {
        #if (FREETYPE_MAJOR > 2 || (FREETYPE_MAJOR == 2 && FREETYPE_MINOR >= 10))
        if (glyphSlot->glyph_index && glyphSlot->bitmap.pixel_mode != FT_PIXEL_MODE_BGRA && !params_.cpalUsed) {
            // REFACTOR
            // Log::instance.logf(Log::TEXTIFY, Log::INFORMATION, "Suspicious pixel mode.");
        }
        #endif
        return std::make_unique<ColorGlyph>();
    } else {
        return std::make_unique<GrayGlyph>();
    }
}

} // namespace textify
