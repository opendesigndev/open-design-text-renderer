#pragma once

#include <string>
#include <vector>

#include "../common/result.hpp"
#include "../text-renderer/base-types.h"

namespace odtr {
namespace codec {

enum Format {
    CODEC_FORMAT_RAW,
    CODEC_FORMAT_XOR,
    CODEC_FORMAT_LZ4,
    CODEC_FORMAT_MONO,
    CODEC_FORMAT_LZ4_MONO,
    CODEC_FORMAT_BAD,
};

const unsigned int SIGNATURE = 'DCVA';


class Codec {
public:
    typedef ::Result<BufferType, bool> Result;

    virtual ~Codec() = default;
    virtual Result encode(const BufferType & input) = 0;
    virtual Result decode(const BufferType & input) = 0;
};

class CodecRaw: public Codec {
public:
    Result encode(const BufferType & input) override;
    Result decode(const BufferType & input) override;
};

class CodecXOR: public Codec {
public:
    Result encode(const BufferType & input) override;
    Result decode(const BufferType & input) override;
};

#if defined(TEXT_RENDERER_USE_LZ4)
class CodecLZ4: public Codec {
public:
    Result encode(const BufferType & input) override;
    Result decode(const BufferType & input) override;

    struct Subheader {
        unsigned int original_length;
    };
};
#endif

#if defined(TEXT_RENDERER_USE_MONOCYPHER)
class CodecMonocypher: public Codec {
public:
    Result encode(const BufferType & input) override;
    Result decode(const BufferType & input) override;
private:
    const char *key = "***REMOVED***";

    struct Subheader {
        uint8_t nonce[ 24 ];
        uint8_t mac[ 16 ];

        void generate();
    };
};
#endif

#if defined(TEXT_RENDERER_USE_LZ4) && defined(TEXT_RENDERER_USE_MONOCYPHER)
class CodecLZ4Mono: public Codec {
private:
    CodecLZ4 lz4;
    CodecMonocypher mono;
public:
    Result encode(const BufferType & input) override;
    Result decode(const BufferType & input) override;
};
#endif


struct Header {
    unsigned int signature;
    unsigned int format;
};



Codec::Result encode(int format, const BufferType & buffer, bool verify);
Codec::Result decode(BufferType & buffer);

} // namespace codec
} // namespace odtr
