#include "FontCodec.h"

#include <cstring>
#include <memory>
#include <random>
#include <chrono>

#ifdef TEXT_RENDERER_USE_LZ4
#include "lz4/lz4.h"
#endif

#ifdef TEXT_RENDERER_USE_MONOCYPHER
extern "C" {
#include "monocypher/monocypher.h"
}
#endif

namespace odtr {
namespace codec {
Codec::Result CodecRaw::encode(const BufferType & input)
{
    BufferType output(input.size());
    memcpy(output.data(), input.data(), input.size());
    return output;
}

Codec::Result CodecRaw::decode(const BufferType & input)
{
    return encode(input);
}

Codec::Result CodecXOR::encode(const BufferType & input)
{
    BufferType output(input.size());

    for (size_t i = 0; i < input.size(); i++) {
        output[ i ] = input[ i ] ^ Byte(97);
    }
    return output;
}

Codec::Result CodecXOR::decode(const BufferType & input)
{
    return encode(input);
}

#if defined(TEXTIFY_USE_LZ4)

Codec::Result CodecLZ4::encode(const BufferType & input)
{
    const int bound = LZ4_compressBound((int)input.size() + sizeof(Subheader));

    BufferType output(bound);

    char *src = (char *)input.data();
    char *dst = (char *)output.data();

    Subheader *subheader = (Subheader *)dst;
    subheader->original_length = unsigned(input.size());

    dst += sizeof(Subheader);

    int compressed = LZ4_compress_default(src, dst, (int)input.size(), bound);

    output.resize(compressed + sizeof(Subheader));

    return output;
}

Codec::Result CodecLZ4::decode(const BufferType & input)
{
    char *src = (char *)input.data();
    Subheader *subheader = (Subheader *)src;

    src += sizeof(Subheader);

    BufferType output(subheader->original_length);
    char *dst = (char *)output.data();

    int compressed_length = (int)input.size() - sizeof(Subheader);
    int decompressed = LZ4_decompress_safe(src, dst, compressed_length, subheader->original_length);

    if (decompressed != subheader->original_length) {
        // REFACTOR
        // Log::instance.log(Log::TEXTIFY, Log::ERROR, "Font decode: invalid LZ4 stream.");
        return false;
    }

    return output;
}

#endif

#if defined(TEXTIFY_USE_MONOCYPHER)

Codec::Result CodecMonocypher::encode(const BufferType & input)
{
    BufferType output(input.size() + sizeof(Subheader));

    uint8_t *src = (uint8_t *)input.data();
    uint8_t *dst = (uint8_t *)output.data();

    Subheader *subheader = (Subheader *)dst;
    subheader->generate();

    dst += sizeof(Subheader);

    crypto_lock(subheader->mac, dst, (const uint8_t *)key, subheader->nonce, src, input.size());

    return output;
}

Codec::Result CodecMonocypher::decode(const BufferType & input)
{
    uint8_t *bytes = (uint8_t *)input.data();
    Subheader *subheader = (Subheader *)bytes;

    bytes += sizeof(Subheader);

    size_t output_length = input.size() - sizeof(Subheader);
    BufferType output(output_length);

    int result = crypto_unlock((uint8_t *)output.data(), (uint8_t *)key, subheader->nonce, subheader->mac, bytes, output_length);
    if (result != 0) {
        // Log::instance.log(Log::TEXTIFY, Log::ERROR, "Font decode: corrupted Monocypher message.");
        return false;
    }

    return output;
}

void CodecMonocypher::Subheader::generate()
{
#if 1
    auto seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::default_random_engine generator((unsigned(seed)));
    std::uniform_int_distribution<unsigned> distribution(0, 255);
    for (auto & byte : nonce) {
        byte = (uint8_t) distribution(generator);
    }
#else
#error !!! Only for testing !!! Used to compare outputs 1:1 with the go utility.
    for (size_t i = 0; i < sizeof(nonce); ++i) {
        nonce[i] = i;
    }
#endif
}

#endif

#if defined(TEXTIFY_USE_LZ4) && defined(TEXTIFY_USE_MONOCYPHER)

Codec::Result CodecLZ4Mono::encode(const BufferType & input)
{
    auto encoded_lz4 = lz4.encode(input);
    if (!encoded_lz4) return false;

    auto encoded_mono = mono.encode(encoded_lz4.value());
    return encoded_mono;
}

Codec::Result CodecLZ4Mono::decode(const BufferType & input)
{
    auto decoded_mono = mono.decode(input);
    if (!decoded_mono) return false;

    auto decoded_lz4 = lz4.decode(decoded_mono.value());
    return decoded_lz4;
}

#endif

static std::unique_ptr<Codec> createCodec(int format)
{
    if (format == CODEC_FORMAT_RAW) return std::make_unique<CodecRaw>();
    if (format == CODEC_FORMAT_XOR) return std::make_unique<CodecXOR>();
#if defined(TEXTIFY_USE_LZ4)
    if (format == CODEC_FORMAT_LZ4) return std::make_unique<CodecLZ4>();
#endif
#if defined(TEXTIFY_USE_MONOCYPHER)
    if (format == CODEC_FORMAT_MONO) return std::make_unique<CodecMonocypher>();
#endif
#if defined(TEXTIFY_USE_LZ4) && defined(TEXTIFY_USE_MONOCYPHER)
    if (format == CODEC_FORMAT_LZ4_MONO) return std::make_unique<CodecLZ4Mono>();
#endif

    return nullptr;
}

static void prependHeader(BufferType & buffer, int format)
{
    buffer.insert(buffer.begin(), sizeof(Header), 0);

    Header *header = (Header *)buffer.data();
    header->signature = SIGNATURE;
    header->format = format;
}

Codec::Result encode(int format, const BufferType & buffer, bool verify)
{
    auto codec = createCodec(format);
    if (!codec) {
        // Log::instance.logf(Log::TEXTIFY, Log::ERROR, "Font codec not recognized: %d", format);
        return false;
    }

    auto result = codec->encode(buffer);
    if (!result) {
        // Log::instance.log(Log::TEXTIFY, Log::ERROR, "Font encoding failed.");
        return false;
    }

    std::vector<Byte> encoded = result.moveValue();

    if (verify) {
        auto decoded = codec->decode(encoded);

        if (!decoded) {
            // Log::instance.log(Log::TEXTIFY, Log::ERROR, "Font verification failed (decode).");
            return false;
        }

        bool identical = buffer == decoded.value();
        if (!identical) {
            // Log::instance.log(Log::TEXTIFY, Log::ERROR, "Font verification failed (mismatch).");
            return false;
        }
    }

    prependHeader(encoded, format);

    return encoded;
}

Codec::Result decode(BufferType & buffer) {
    Byte *bytes = buffer.data();
    const Header *header = (Header *)bytes;

    if (header->signature == SIGNATURE && buffer.size() >= sizeof(Header)) {
        auto codec = createCodec(header->format);
        if ( codec ) {
            buffer.erase(buffer.begin(), buffer.begin() + sizeof(Header));
            return codec->decode(buffer);
        } else {
            /* Unknown codec or broken header */
            return false;
        }
    } else {
        /* Not our container format, treat as raw data */
        return buffer;
    }
}

} // namespace codec
} // namespace odtr
