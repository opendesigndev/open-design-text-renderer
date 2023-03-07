#include "FontStorage.h"
#include "FontCodec.h"


namespace odtr {


Byte *FontStorage::alloc(const std::string& name, std::size_t size)
{
    auto [it, _] = storage_.insert(std::make_pair(name, Item { BufferType {}, false }));

    it->second.buffer.resize(size);
    it->second.decoded = false;

    return it->second.buffer.data();
}

BufferView FontStorage::get(const std::string& name)
{
    auto it = storage_.find(name);

    if (it != std::end(storage_) && it->second.buffer.size()) {
        if (!it->second.decoded) {
            decode(it->second);
        }

        if (it->second.success) {
            return BufferView(it->second.buffer.data(), it->second.buffer.size());
        }
    }

    return BufferView::createEmpty();
}

bool FontStorage::contains(const std::string& name) const
{
    return storage_.find(name) != std::end(storage_);
}

void FontStorage::decode(Item &item)
{
    auto result = codec::decode(item.buffer);
    if ( result ) {
        item.buffer = result.value();
        item.success = true;
    } else {
        item.success = false;
    }
    item.decoded = true;
}

} // namespace odtr

