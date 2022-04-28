#include "utils.h"
#include "textify/textify-api.h"

#include <array>
#include <cmath>

namespace textify {
namespace utils {

// the reinterpret_cast bellow is kind of dangerous, however, these fundamental types most likely won't ever change
textify::FRectangle castFRectangle(const compat::FRectangle& r)
{
    return *reinterpret_cast<const textify::FRectangle*>(&r);
}

textify::Rectangle castRectangle(const compat::Rectangle& r)
{
    return *reinterpret_cast<const textify::Rectangle*>(&r);
}

textify::Matrix3f castMatrix(const compat::Matrix3f& m)
{
    return *reinterpret_cast<const textify::Matrix3f*>(&m);
}

compat::FRectangle toFRectangle(const compat::Rectangle& r)
{
    return {(float)r.l, (float)r.t, (float)r.w, (float)r.h};
}

compat::FRectangle transform(const compat::FRectangle& rect, const compat::Matrix3f& m) {

    auto asVec3 = [](float x, float y) {
        return compat::Vector3f { x, y, 1.f };
    };

    auto lt = asVec3(rect.l, rect.t);
    auto rt = asVec3(rect.l + rect.w, rect.t);
    auto lb = asVec3(rect.l, rect.t + rect.h);
    auto rb = asVec3(rect.l + rect.w, rect.t + rect.h);

    std::array<compat::Vector3f,4> points = {{lt, rt, lb, rb}};
    std::array<compat::Vector3f,4> transformed;

    for (auto i = 0ul; i < 4; ++i) {
          transformed[i] = m * points[i];
    }

    auto l = std::numeric_limits<float>::max();
    auto t = std::numeric_limits<float>::max();
    auto r = std::numeric_limits<float>::lowest();
    auto b = std::numeric_limits<float>::lowest();

    for (const auto& p : transformed) {
        auto x = p.x;
        auto y = p.y;

        l = (x < l) ? x : l;
        t = (y < t) ? y : t;
        r = (x > r) ? x : r;
        b = (y > b) ? y : b;
    }

    return compat::FRectangle { l, t, r-l, b-t };
}

compat::Rectangle outerRect(const compat::FRectangle& rect)
{
    compat::Rectangle result;
    result.l = (int) floorf(rect.l);
    result.t = (int) floorf(rect.t);
    result.w = (int) ceilf(rect.l+rect.w)-result.l;
    result.h = (int) ceilf(rect.t+rect.h)-result.t;
    return result;
}

} // namespace utils
} // namespace textify
