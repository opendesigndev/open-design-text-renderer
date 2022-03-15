#include <cstdint>
#include <utility>

namespace textify {
namespace otf {

template <typename Impl>
class Storage
{
public:
    explicit Storage(Impl&& impl)
        : impl_(std::move(impl))
    {
    }

    template <typename T>
    T read(std::int64_t offset=0)
    {
        return impl_.template read<T>(offset);
    }

private:
    Impl impl_;
};

} // namespace otf
} // namespace textify
