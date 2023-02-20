#pragma once

#include "compat/basic-types.h"

#include <unordered_map>
#include <string>
#include <vector>

namespace textify {

typedef compat::Vector2i IVec2;
typedef compat::Vector2f FVec2;
typedef IVec2 IPoint2;
typedef IVec2 IDims2;
typedef std::unordered_map<std::string, bool> FontFallbackTable;

typedef std::uint8_t Byte;
typedef std::vector<Byte> BufferType;

typedef float RenderScale;

} // namespace textify
