#include "Block.h"

namespace odtr {
namespace unicode {

using namespace compat;

Block::Block(const char* name, qchar from, qchar to)
    : name_(name),
      from_(from),
      to_(to)
{ }

// ...

bool Block::contains(qchar cp) const
{
    return from_ <= cp && cp <= to_;
}

} // namespace unicode
} // namespace odtr
