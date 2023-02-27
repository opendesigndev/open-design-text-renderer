#include "EmojiTable.h"
#include <algorithm>

namespace odtr {
namespace unicode {

using namespace compat;

const EmojiTable& EmojiTable::instance()
{
    static EmojiTable inst = EmojiTable::create();
    return inst;
}

bool EmojiTable::lookup(qchar cp) const
{
    auto end = std::end(table_);
    auto it = std::lower_bound(std::begin(table_), end, cp);

    return it != end && it->contains(cp);
}

std::vector<qchar> EmojiTable::listAll(std::size_t limit) const
{
    std::vector<qchar> emojiCodePoints;

    auto i = 0ul;
    for (const auto& range : table_) {
        for (auto cp = range.from; cp <= range.to; ++cp) {
            emojiCodePoints.emplace_back(cp);
            if (limit && ++i >= limit) {
                goto end;
            }
        }
    }
end:

    return emojiCodePoints;
}

void EmojiTable::add(qchar cp)
{
    table_.emplace_back(RangeRec{cp, cp});
}

void EmojiTable::add(qchar from, qchar to)
{
    table_.emplace_back(RangeRec{from, to});
}

bool EmojiTable::RangeRec::contains(qchar cp) const
{
    return from <= cp && cp <= to;
}

bool EmojiTable::RangeRec::operator<(qchar cp) const
{
    return to < cp;
}

} // namespace unicode
} // namespace odtr
