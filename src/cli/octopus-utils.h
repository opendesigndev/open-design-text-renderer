#pragma once

#include "compat/basic-types.h"
#include "octopus/general.h"

namespace octopus {
    struct Color;
}

namespace odtr {
namespace cli {

compat::Matrix3f convertMatrix(const double octopusMatrix[6]);
compat::Color convertColor(const octopus::Color& octopusColor);

}
}
