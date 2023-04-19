#pragma once

#include "types.h"

#include "../common/hash_utils.hpp"
#include "../common/lexical_cast.hpp"

namespace std {
template <>
struct hash<ReportedFontItem>
{
    std::size_t operator()(const ReportedFontItem& item) const
    {
        std::size_t seed;
        hash_combine(seed, item.fontName);
        hash_combine(seed, item.reason);
        return seed;
    }
};
}; // namespace std

inline bool operator==(const ReportedFontItem& lhs, const ReportedFontItem& rhs)
{
    return lhs.fontName == rhs.fontName && lhs.reason == rhs.reason;
}

template <>
inline std::string lexical_cast(const ReportedFontReason& reason)
{
    switch (reason) {
        case ReportedFontReason::MISSING_GLYPH:
            return "missing glyph";
        case ReportedFontReason::POSTSCRIPTNAME_MISMATCH:
            return "postScript name mismatch";
        case ReportedFontReason::NO_DATA:
            return "no data";
        case ReportedFontReason::LOAD_FAILED:
            return "load failed";
        default:
            return "";
    }
}

template <>
inline std::string lexical_cast(const ReportedFontItem& item)
{
    return item.fontName + "(" + lexical_cast(item.reason) + ")";
}
