#pragma once

#include "../common/buffer_view.h"
#include "../text-renderer/base-types.h"

#include <string>
#include <vector>
#include <unordered_map>


namespace odtr {

class FontStorage
{
public:
    using Key = std::string;

    Byte *alloc(const Key& name, std::size_t size);

    BufferView get(const Key& name);

    void mark(const Key& name, bool success);

    bool contains(const Key& name) const;

private:
    struct Item {
        BufferType buffer;
        bool success;
    };

    std::unordered_map<Key, Item> storage_;
};

} // namespace odtr
