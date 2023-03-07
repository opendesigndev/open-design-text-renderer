#pragma once

#include "compat/basic-types.h"
#include "fmt/core.h"

namespace odtr {
namespace utils {

template <typename RectType>
void printRect(const std::string& label, const RectType& r) {
    fmt::print("{} [{},{}] ({} x {})\n", label, r.l, r.t, r.w, r.h);
}

} // namespace utils
} // namespace odtr

