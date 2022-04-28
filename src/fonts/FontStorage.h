#pragma once

#include "common/buffer_view.h"
#include "textify/base-types.h"

#include <string>
#include <unordered_map>
#include <vector>


namespace textify {

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
        bool decoded;
        bool success;
    };

    using StorageType = std::unordered_map<std::string, Item>;

    StorageType storage_;

    void decode( Item & item );
};

} // namespace textify
