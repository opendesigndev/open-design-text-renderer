#pragma once

#include "compat/basic-types.h"
#include "textify/textify-api.h"

namespace textify {
namespace utils {

textify::FRectangle castFRectangle(const compat::FRectangle& r);
textify::Rectangle castRectangle(const compat::Rectangle& r);
textify::Matrix3f castMatrix(const compat::Matrix3f& m);

compat::FRectangle toFRectangle(const compat::Rectangle& r);

compat::FRectangle transform(const compat::FRectangle& rect, const compat::Matrix3f& m);

compat::Rectangle outerRect(const compat::FRectangle& rect);


} // namespace utils
} // namespace textify
