#pragma once

#include <string>
#include <unordered_set>

namespace textify {
namespace otf {

namespace feature {

static const char* KERN = "kern";   // kerning
static const char* LIGA = "liga";   // standard ligatures
static const char* DLIG = "dlig";   // discretionary ligatures
static const char* HLIG = "hlig";   // historical ligatures
static const char* CLIG = "clig";   // contextual ligatures / alternates

}

class Features
{
public:
    using SetType = std::unordered_set<std::string>;

    Features() = default;

    explicit Features(SetType&& featureSet);

    bool hasFeature(const std::string& featureTag) const;

    void merge(const Features& features);

private:
    SetType features_;
};

} // namespace otf
} // namespace textify
