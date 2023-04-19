#pragma once

#include "Features.h"

#include "../common/buffer_view.h"
#include "../common/result.hpp"

#include "../text-renderer/base.h"

#include <string>
#include <unordered_set>

namespace odtr {
namespace otf {

using FeatureSet = std::unordered_set<std::string>;
using FeaturesResult = Result<Features, bool>;

FeaturesResult listFeatures(BufferView buffer);
FeaturesResult listFeatures(const compat::byte* ptr, std::size_t length);
FeaturesResult listFeatures(const std::string& filename);

} // namespace otf
} // namespace odtr
