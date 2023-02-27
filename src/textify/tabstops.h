#pragma once

#include "base.h"
#include "text-format.h"

#include "compat/basic-types.h"

#include <optional>

namespace odtr {

inline static std::optional<spacing> newlineOffset(const TabStops& tabStops)
{
    if (tabStops.size() > 1) {
        return tabStops[1];
    }
    return {};
}

}
