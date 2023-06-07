#include "FontStorage.h"


namespace odtr {

Byte *FontStorage::alloc(const FontStorage::Key& name, std::size_t size) {
    auto [it, _] = storage_.insert(std::make_pair(name, Item { BufferType {}, false }));

    it->second.buffer.resize(size);

    return it->second.buffer.data();
}

BufferView FontStorage::get(const FontStorage::Key& name) {
    auto it = storage_.find(name);

    if (it != std::end(storage_) && it->second.buffer.size()) {
        if (it->second.success) {
            return BufferView(it->second.buffer.data(), it->second.buffer.size());
        }
    }

    return BufferView::createEmpty();
}

void FontStorage::mark(const FontStorage::Key& name, bool success) {
    auto it = storage_.find(name);

    if (it != std::end(storage_) && it->second.buffer.size()) {
        it->second.success = success;
    }
}

bool FontStorage::contains(const FontStorage::Key& name) const {
    return storage_.find(name) != std::end(storage_);
}

} // namespace odtr
