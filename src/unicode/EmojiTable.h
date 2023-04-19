#pragma once

#define FULL_EMOJI_TABLE

#include "../compat/basic-types.h"

#include <vector>

namespace odtr {
namespace unicode {

class EmojiTable
{
public:
    EmojiTable(EmojiTable&&) = default;
    EmojiTable(const EmojiTable&) = delete;

    static const EmojiTable& instance();

    bool lookup(compat::qchar cp) const;

    std::vector<compat::qchar> listAll(std::size_t limit = 0) const;

private:
    EmojiTable() = default;

    static EmojiTable create();

    struct RangeRec
    {
        compat::qchar from;
        compat::qchar to;

        bool contains(compat::qchar cp) const;

        bool operator<(compat::qchar cp) const;
    };

    using Table = std::vector<RangeRec>;

    void add(compat::qchar cp);
    void add(compat::qchar from, compat::qchar to);

    Table table_;
};

} // namespace unicode
} // namespace odtr
