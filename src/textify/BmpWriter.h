
#pragma once

#include "compat/Bitmap.hpp"

namespace textify {
namespace priv {

class BmpWriter
{
public:
    explicit BmpWriter(compat::BitmapRGBA &bitmap);

    bool checkV(int y);
    bool checkH(int x);

    void write(int x, int y, compat::Pixel32 color);

private:
    compat::BitmapRGBA &bitmap_;
};

} // namespace priv
} // namespace textify
