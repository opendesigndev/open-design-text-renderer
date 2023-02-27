
#include "affine-transform.h"

#include <array>
#include <cmath>
#include <limits>

namespace odtr {
namespace compat {

Matrix3f rotationMatrix(float angle, Vector2f center) {
    float sa = sinf(angle);
    float ca = cosf(angle);
    return Matrix3f { {
        { ca, sa, 0.f },
        { -sa, ca, 0.f },
        {
            -center.x*ca + center.y*sa + center.x,
            -center.x*sa - center.y*ca + center.y,
            1.f
        }
    } };
}

Matrix3f scaleMatrix(float xScale, float yScale, Vector2f center) {
    return Matrix3f { {
        { xScale, 0.f, 0.f },
        { 0.f, yScale, 0.f },
        {
            -center.x*xScale + center.x,
            -center.y*yScale + center.y,
            1.f
        }
    } };
}

Matrix3f translationMatrix(Vector2f translation) {
    return Matrix3f { {
        { 1.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f },
        {
            translation.x,
            translation.y,
            1.f
        }
    } };
}

FRectangle transform(const FRectangle& rect, const Matrix3f& m) {

    auto asVec3 = [](float x, float y) {
        return Vector3f { x, y, 1.f };
    };

    auto lt = asVec3(rect.l, rect.t);
    auto rt = asVec3(rect.l + rect.w, rect.t);
    auto lb = asVec3(rect.l, rect.t + rect.h);
    auto rb = asVec3(rect.l + rect.w, rect.t + rect.h);

    std::array<Vector3f,4> points = {{lt, rt, lb, rb}};
    std::array<Vector3f,4> transformed;

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

    return FRectangle { l, t, r-l, b-t };
}

bool isLinearIdentity(const Matrix3f& m)
{
    return m[0][0] == 1 && m[0][1] == 0 && m[1][0] == 0 && m[1][1] == 1;
}

bool hasTranslation(const Matrix3f& m)
{
    return m[2][0] != 0 || m[2][1] != 0;
}

float deconstructRotation(const Matrix3f& transformation, bool &flip) {
    flip = transformation[0][0]*transformation[1][1]-transformation[0][1]*transformation[1][0] < 0.f;
    float flipSign = flip ? -1.f : +1.f;
    Vector3f v = transformation*Vector3f { flipSign, 0.f, 0.f };
    return -flipSign*atan2f(v.y, v.x);
}

} // namespace compat
} // namespace odtr
