#pragma once

#include "common/buffer_view.h"
#include "common/result.hpp"

#include <cstdint>

namespace textify {
namespace otf {

class StorageBufferImpl
{
public:
    static Result<StorageBufferImpl,bool> create(BufferView buffer);

    template <typename T>
    T read(std::int64_t offset);

private:
    StorageBufferImpl(BufferView::BytePtr base, std::uint64_t max_offset);

    BufferView::BytePtr base_;
    std::uint64_t max_offset_;
    std::int64_t offset_;
};

template <typename T>
T StorageBufferImpl::read(std::int64_t offset)
{
    constexpr auto size = sizeof(T);
    auto curOffset = offset_;

    if (offset) {
        curOffset = offset;
        offset_ = offset + size;
    } else {
        offset_ += size;
    }

    if (curOffset < 0 || std::uint64_t(curOffset) >= max_offset_) {
        // buffer over(under)flow
        return T{};
    }

    return *reinterpret_cast<T*>(base_ + curOffset);
}

} // namespace otf
} // namespace textify
