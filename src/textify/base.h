#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include FT_OUTLINE_H
#include FT_ADVANCES_H
#include FT_GLYPH_H
#include FT_BBOX_H
#include <hb-ft.h>
#include <hb.h>

#include "base-types.h"
#include "text-format.h"
#include "types.h"

#include <unicode/unistr.h>

#include <string>
#include <unordered_map>

namespace textify {

typedef std::unordered_map<std::string, bool> FlagsTable;

typedef icu::UnicodeString ustring;

inline static bool isWhitespace(compat::qchar c)
{
    return c == (compat::qchar)' ' || c == (compat::qchar)'\t' || c == 0x2028;
}

inline static bool isTabStop(compat::qchar c)
{
    return c == (compat::qchar)'\t';
}

} // namespace textify
