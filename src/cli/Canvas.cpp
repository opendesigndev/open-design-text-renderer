#include "Canvas.h"

#include "compat/basic-types.h"
#include "compat/png.h"
#include "compat/pixel-conversion.h"

#include "fmt/core.h"

#include "utils/fmt.h"
#include "textify/textify-api.h"

namespace textify {
namespace cli {

Canvas::Canvas(int width, int height)
    : data_((uint32_t*)calloc(width * height, 4)),
      bitmapView_(compat::BitmapRGBA::WRAP_NO_OWN, data_, width, height)
{ }

Canvas::~Canvas()
{
    free(data_);
}

void Canvas::blendCanvas(const Canvas& canvas, const compat::Rectangle& bounds, const compat::Matrix3f& m)
{
    blendBitmap(canvas.bitmapView_, bounds, m);
}

void Canvas::blendBitmap(const compat::BitmapRGBA& bitmap, const compat::Rectangle& bounds, const compat::Matrix3f& m)
{
    auto asVec3 = [](int x, int y) {
        return compat::Vector3f { (float)x, (float)y, 1.0f };
    };

    for (int y = 0; y < bitmap.height(); ++y) {
        for (int x = 0; x < bitmap.width(); ++x) {
            auto pt = m * asVec3(x + bounds.l, y + bounds.t);

            auto dx = (int)pt.x; // - (int)tr.l;
            auto dy = (int)pt.y; // - (int)tr.t;

            auto c = compat::pixelToColor(bitmap(x,y));
            blend(dx, dy, c);
        }
    }
}

void Canvas::drawRect(const compat::Rectangle& rect, const compat::Color& color)
{
    auto pixel = compat::premultiplyPixel(compat::colorToPixel(color));

    auto y1 = std::min(rect.t + rect.h, bitmapView_.height());
    auto x1 = std::min(rect.l + rect.w, bitmapView_.width());

    for (int y = rect.t; y < y1; ++y) {
        for (int x = rect.l; x < x1; ++x) {
            blend(x, y, color);
        }
    }
}

void Canvas::fill(const compat::Color& color)
{
    // canvas always contain premultiplied values
    auto pixel = compat::premultiplyPixel(compat::colorToPixel(color));
    for (int y = 0; y < bitmapView_.height(); ++y) {
        for (int x = 0; x < bitmapView_.width(); ++x) {
            bitmapView_(x, y) = pixel;
        }
    }
}

bool Canvas::save(const std::string& filename)
{
    compat::BitmapRGBA bmp = bitmapView_;

    const compat::Pixel32* end = bmp.pixels() + bmp.width() * bmp.height();
    for (compat::Pixel32* p = bmp.pixels(); p < end; ++p)
        *p = compat::unpremultiplyPixel(*p);

    return savePngImage(bmp, filename.c_str());
}

void Canvas::blend(int x, int y, const compat::Color& src)
{
    if (x >= bitmapView_.width() || y >= bitmapView_.height()) {
        return;
    }

    /*
    // blending for src and dest having non-premultiplied alpha
    auto r = src.a * src.r + (1 - src.a) * dst.a * dst.r;
    auto g = src.a * src.g + (1 - src.a) * dst.a * dst.g;
    auto b = src.a * src.b + (1 - src.a) * dst.a * dst.b;
    */

    auto dst = compat::pixelToColor(bitmapView_(x, y));

    auto r = src.r + (1 - src.a) * dst.r;
    auto g = src.g + (1 - src.a) * dst.g;
    auto b = src.b + (1 - src.a) * dst.b;

    auto a = src.a + (1 - src.a) * dst.a;

    bitmapView_(x, y) = compat::colorToPixel(compat::Color::makeRGBA(r, g, b, a));
}

}
}
