#pragma once

#include "../common/buffer_view.h"
#include "../text-renderer/base-types.h"

#include <string>
#include <unordered_map>
#include <vector>


namespace odtr {

typedef std::vector<Byte> BufferType;


class FontStorage
{
public:
    Byte *alloc(const std::string& name, std::size_t size);

    BufferView get(const std::string& name);

    bool contains(const std::string& name) const;

private:

    struct Item {
        BufferType buffer;
        bool success;
    };

    using StorageType = std::unordered_map<std::string, Item>;

    StorageType storage_;
};

} // namespace odtr
