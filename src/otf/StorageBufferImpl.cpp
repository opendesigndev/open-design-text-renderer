#include "StorageBufferImpl.h"

namespace textify {
namespace otf {

Result<StorageBufferImpl,bool> StorageBufferImpl::create(BufferView buffer)
{
    return StorageBufferImpl(buffer.data(), buffer.size());
}

StorageBufferImpl::StorageBufferImpl(BufferView::BytePtr base, std::uint64_t max_offset)
    : base_(base),
      max_offset_(max_offset),
      offset_(0)
{ }

} // namespace otf
} // namespace textify
