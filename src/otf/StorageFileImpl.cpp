#include "StorageFileImpl.h"

namespace odtr {
namespace otf {

Result<StorageFileImpl,bool> StorageFileImpl::create(const std::string& filename)
{
    StorageFileImpl file;
    if (!file.open(filename)) {
        return false;
    }

    return file;
}

StorageFileImpl::~StorageFileImpl()
{
    if (close_) {
        fclose(file_);
    }
}

StorageFileImpl::StorageFileImpl()
    : file_(nullptr),
      close_(true)
{ }

bool StorageFileImpl::open(const std::string& filename)
{
    file_ = fopen(filename.c_str(), "rb");
    return file_ != nullptr;
}

} // namespace otf
} // namespace odtr
