#pragma once

#include "base-types.h"

#include <string>
#include <vector>

using FacesNames = std::vector<std::string>;

enum class ReportedFontReason
{
    MISSING_GLYPH,
    POSTSCRIPTNAME_MISMATCH,
    NO_DATA,
    LOAD_FAILED
};

struct ReportedFontItem
{
    std::string fontName;
    ReportedFontReason reason;
};
using ReportedFonts = std::vector<ReportedFontItem>;
