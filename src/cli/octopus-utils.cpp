#include "cli/octopus-utils.h"
#include "compat/basic-types.h"

namespace odtr {
namespace cli {

compat::Matrix3f convertMatrix(const double octopusMatrix[6])
{
    auto m = compat::Matrix3f::identity;

    m[0][0] = octopusMatrix[0];
    m[0][1] = octopusMatrix[1];
    m[1][0] = octopusMatrix[2];
    m[1][1] = octopusMatrix[3];
    m[2][0] = octopusMatrix[4];
    m[2][1] = octopusMatrix[5];

    return m;
}

compat::Color convertColor(const octopus::Color& octopusColor)
{
    return compat::Color::makeRGBA(octopusColor.r, octopusColor.g, octopusColor.b, octopusColor.a);
}

}
} // namespace odtr
