#include "FontStorage.h"


namespace odtr {


Byte *FontStorage::alloc(const std::string& name, std::size_t size)
{
    auto [it, _] = storage_.insert(std::make_pair(name, Item { BufferType {}, false }));

    it->second.buffer.resize(size);

    return it->second.buffer.data();
}

BufferView FontStorage::get(const std::string& name)
{
    auto it = storage_.find(name);

    if (it != std::end(storage_) && it->second.buffer.size()) {
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

} // namespace odtr
