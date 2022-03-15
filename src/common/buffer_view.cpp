#include "buffer_view.h"

BufferView::BufferView(BytePtr data, std::size_t size) : data_(data), size_(size)
{
}

BufferView BufferView::createEmpty()
{
    return BufferView(nullptr, 0);
}

BufferView::BytePtr BufferView::data() const
{
    return data_;
}

std::size_t BufferView::size() const
{
    return size_;
}

BufferView::operator bool() const
{
    return data_ && size_;
}
