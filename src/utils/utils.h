#pragma once

#include "compat/basic-types.h"
#include "textify/textify-api.h"

namespace odtr {
namespace utils {

odtr::FRectangle castFRectangle(const compat::FRectangle& r);
odtr::Rectangle castRectangle(const compat::Rectangle& r);
odtr::Matrix3f castMatrix(const compat::Matrix3f& m);

compat::FRectangle toFRectangle(const compat::Rectangle& r);

compat::FRectangle transform(const compat::FRectangle& rect, const compat::Matrix3f& m);

compat::Rectangle outerRect(const compat::FRectangle& rect);

compat::FRectangle scaleRect(const compat::FRectangle& rect, float scale);

} // namespace utils
} // namespace odtr
