#pragma once

#include "compat/Bitmap.hpp"
#include "compat/basic-types.h"

#include <cstdint>
#include <string>

namespace odtr {
namespace cli {

class Canvas
{
public:
    Canvas(int width, int height);
    ~Canvas();

    void blendCanvas(const Canvas& canvas, const compat::Rectangle& bounds, const compat::Matrix3f& m);

    void blendBitmap(const compat::BitmapRGBA& bitmap, const compat::Rectangle& bounds, const compat::Matrix3f& m);
    void blendBitmap2(const compat::BitmapRGBA& bitmap, const compat::Rectangle& bounds, const compat::Matrix3f& m);

    void drawRect(const compat::Rectangle& rect, const compat::Color& color);
    void fill(const compat::Color& color);

    bool save(const std::string& filename);

private:
    inline void blend(int x, int y, const compat::Color& color);
    void blendPremul(int x, int y, const compat::Color& src);

    uint32_t* data_;
    compat::BitmapRGBA bitmapView_;
};

}
} // namespace odtr
