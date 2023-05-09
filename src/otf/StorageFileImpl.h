#pragma once

#include "../common/buffer_view.h"
#include "../common/result.hpp"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <string>

namespace odtr {
namespace otf {

class StorageFileImpl
{
public:
    static Result<StorageFileImpl,bool> create(const std::string& filename);

    StorageFileImpl(const StorageFileImpl&) = delete;

    StorageFileImpl(StorageFileImpl&& rhs)
    {
        file_ = rhs.file_;
        close_ = true;

        rhs.close_ = false;
    }

    ~StorageFileImpl();

    template <typename T>
    T read(off_t offset)
    {
        constexpr auto size = sizeof(T);
        T data;

        if (offset) {
            fseek(file_, offset, SEEK_SET);
        }

        auto blocksRead = fread(&data, size, 1, file_);

        return data;
    }

private:
    StorageFileImpl();

    bool open(const std::string& filename);


    FILE* file_;
    bool close_;
};

} // namespace otf
} // namespace odtr

