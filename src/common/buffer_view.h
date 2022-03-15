#pragma once

#include <cstddef>
#include <cstdint>

/**
 * BufferView is non-owning wrapper around a chunk of memory
 */
class BufferView
{
public:
    using Byte = std::uint8_t;
    using BytePtr = Byte*;

    BufferView(BytePtr data, std::size_t size);

    static BufferView createEmpty();

    BytePtr data() const;

    std::size_t size() const;

    /* implicit */ operator bool() const;

private:
    BytePtr data_;
    std::size_t size_;
};
