#pragma once

#include "compat/basic-types.h"

namespace textify {
namespace unicode {

class Block
{
public:
    Block(const char* name, compat::qchar from, compat::qchar to);

    bool contains(compat::qchar cp) const;

private:
    const char* name_;
    compat::qchar from_;
    compat::qchar to_;
};

} // namespace unicode
} // namespace textify
