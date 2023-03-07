#include "Features.h"

#include <iterator>

namespace odtr {
namespace otf {

Features::Features(SetType&& featureSet)
    : features_(std::move(featureSet))
{
}

bool Features::hasFeature(const std::string& featureTag) const
{
    return features_.find(featureTag) != std::end(features_);
}

void Features::merge(const Features& features)
{
    auto beg = std::begin(features.features_);
    auto end = std::end(features.features_);
    std::copy(beg, end, std::inserter(features_, std::end(features_)));
}

} // namespace otf
} // namespace odtr
