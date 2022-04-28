#include "otf.h"

#include "Storage.hpp"
#include "StorageBufferImpl.h"
#include "StorageFileImpl.h"

#include <array>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace textify {
namespace otf {

using StorageType = Storage<StorageBufferImpl>;
// using StorageType = Storage<StorageFileImpl>;

using int8  = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;

using uint8  = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;

using Tag = std::array<uint8, 4>;

using Offset16 = uint16;
using Offset32 = uint32;

template <typename T>
T swap_bytes(T value);

template <>
uint32 swap_bytes(uint32 val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}

template <>
int32 swap_bytes(int32 val)
{
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | ((val >> 16) & 0xFFFF);
}

template <>
uint16 swap_bytes(uint16 val)
{
    return (val << 8) | (val >> 8 );
}

template <>
int16 swap_bytes(int16 val)
{
    return (val << 8) | ((val >> 8) & 0xFF);
}

bool operator==(const Tag& tag, const char* name)
{
    return std::get<0>(tag) == name[0] &&
           std::get<1>(tag) == name[1] &&
           std::get<2>(tag) == name[2] &&
           std::get<3>(tag) == name[3];
}

std::string tagAsString(const Tag& tag)
{
    const char tagStr[] = {
        (char)std::get<0>(tag),
        (char)std::get<1>(tag),
        (char)std::get<2>(tag),
        (char)std::get<3>(tag),
        '\0',
    };

    return tagStr;
}

/**
 * Big Endian wrapper class for type T.
 */
template <typename T>
class BE
{
public:
    BE() = default;
    BE(const BE<T>&) = default;

    BE(T val)
        : value_(val)
    { }

    operator T() const
    {
        return swap_bytes(value_);
    }

    T value() const
    {
        return swap_bytes(value_);
    }
private:
    T value_;
};

struct OffsetTable
{
    BE<uint32> sfntVersion;
    BE<uint16> numTables;
    BE<uint16> searchRange;
    BE<uint16> entrySelector;
    BE<uint16> rangeShift;
};

struct Table
{
    Tag      tag;
    BE<uint32>   checkSum;
    BE<Offset32> offset;
    BE<uint32>   length;
};

struct GPOS_Header
{
    BE<uint16>   majorVersion;
    BE<uint16>   minorVersion;
    BE<Offset16> scriptListOffset;
    BE<Offset16> featureListOffset;
    BE<Offset16> lookupListOffset;
    /* BE<Offset32> featureVariationsOffset; only ver. 1.1 */
};

struct GSUB_Header
{
    BE<uint16>   majorVersion;
    BE<uint16>   minorVersion;
    BE<Offset16> scriptListOffset;
    BE<Offset16> featureListOffset;
    BE<Offset16> lookupListOffset;
    /* BE<Offset32> featureVariationsOffset; only ver. 1.1 */
};

struct FeatureRecord
{
    Tag      featureTag;
    BE<Offset16> featureOffset;
};

struct FeatureList
{
    BE<uint16>                 featureCount;
    std::vector<FeatureRecord> featureRecords;
};

struct FeatureItemHeader
{
    BE<Offset16> featureParams;
    BE<uint16> lookupIndexCount;
};

struct FeatureItem
{
    FeatureItemHeader   auxHeader;
    std::vector<BE<uint16>> lookupListIndices;
};

FeatureList readFeatureList(StorageType& storage, Offset32 offset)
{
    FeatureList featureList;
    featureList.featureCount = storage.read<uint16>(offset);

    for (auto i = 0u; i < featureList.featureCount; ++i) {
        featureList.featureRecords.emplace_back(storage.read<FeatureRecord>());
    }

    return featureList;
}

void printTag(const Tag& tag) {
    printf("'%c%c%c%c'",
            std::get<0>(tag),
            std::get<1>(tag),
            std::get<2>(tag),
            std::get<3>(tag));
}

void printOffsetTable(const OffsetTable& table)
{
    printf("sfnt version: 0x%X\n", (uint32) table.sfntVersion);
    printf("num tables: %d\n", (uint16)table.numTables);
}

void printTableRecord(const Table& table)
{
    printf("tag:"); printTag(table.tag); printf("\n");
    printf("checksum: %x\n", (uint32)table.checkSum);
    printf("offset %u length: %u\n", (Offset32)table.offset, (uint32)table.length);
}

FeatureItem parseFeatureItem(StorageType& storage, Offset32 offset)
{
    FeatureItem featureItem;
    auto featureItemHeader = storage.read<FeatureItemHeader>(offset);

    featureItem.auxHeader = featureItemHeader;

    for (auto i = 0u; i < featureItem.auxHeader.lookupIndexCount; i++) {
        featureItem.lookupListIndices.emplace_back(storage.read<uint16>());
    }

    return featureItem;
};

template <typename Table>
Features parseFeatures(StorageType& storage, Offset32 offset)
{
    Features::SetType features;

    auto header = storage.read<Table>(offset);

    Offset32 featureListOffset = offset + header.featureListOffset;

    auto featureList = readFeatureList(storage, featureListOffset);

    for (const auto& feature : featureList.featureRecords) {

        Offset32 featureOffset = featureListOffset + feature.featureOffset;
        auto featureItem = parseFeatureItem(storage, featureOffset);

        auto tag = tagAsString(feature.featureTag);

        // printf("* tag %s offset: %u lookup index count: %u\n", tag.c_str(), (uint32)feature.featureOffset, (uint16)featureItem.auxHeader.lookupIndexCount);

        features.emplace(std::move(tag));
    }

    return Features(std::move(features));
}

FeatureSet mergeFeatures(const FeatureSet& a, const FeatureSet& b)
{
    auto merged = a;
    std::copy(std::begin(b), std::end(b), std::inserter(merged, merged.end()));

    return merged;
}

FeaturesResult listFeaturesFromStorage(StorageType& storage) {
    auto offsetTable = storage.read<OffsetTable>();

    // printOffsetTable(offsetTable);

    if (offsetTable.sfntVersion != 0x10000 && offsetTable.sfntVersion != 0x4F54544F) {
        // data doesn't represent OpenType formatted font
        return false;
    }

    Offset32 gpos_offset = 0, gsub_offset = 0;

    for (auto i = 0ul; i < offsetTable.numTables; ++i) {
        auto tableRecord = storage.read<Table>();
        // printTableRecord(tableRecord);
        if (tableRecord.tag == "GPOS") {
            gpos_offset = tableRecord.offset;
        } else if (tableRecord.tag == "GSUB") {
            gsub_offset = tableRecord.offset;
        }
    }

    Features features;

    if (gpos_offset) {
        features.merge(parseFeatures<GPOS_Header>(storage, gpos_offset));
    }

    if (gsub_offset) {
        features.merge(parseFeatures<GSUB_Header>(storage, gsub_offset));
    }

    return features;
}

FeaturesResult listFeatures(BufferView buffer)
{
    auto storageResult = StorageBufferImpl::create(buffer);
    if (!storageResult) {
        return false;
    }

    Storage<StorageBufferImpl> storage(storageResult.moveValue());

    return listFeaturesFromStorage(storage);
}

FeaturesResult listFeatures(const compat::byte* ptr, std::size_t length)
{
    return listFeatures(BufferView((BufferView::BytePtr)ptr, length));
}

FeaturesResult listFeatures(const std::string& filename)
{
    std::ifstream ifs(filename, std::ios::binary);

    if (!ifs.is_open()) {
        return false;
    }

    ifs.seekg(0, std::ios::end);
    std::size_t size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> buffer(size);

    ifs.read((char*)buffer.data(), size);

    return listFeatures(BufferView(buffer.data(), buffer.size()));
}

} // namespace otf
} // namespace textify


/*
bool listFeatures_FileStorage(const std::string& filename) {

    auto fileResult = StorageFileImpl::create(filename);
    if (!fileResult) {
        return false;
    }

    Storage<StorageFileImpl> storage(fileResult.moveValue());
    FeatureSet features;
    // auto features = listFeaturesFromStorage(storage);

    for (const auto& feature : features) {
        printf("%s ", feature.c_str());
    }
    printf("\n");

    return true;
}

int main(int argc, const char* argv[]) {

    std::vector<std::string> args(argv, argv + argc);

    printf("OTF Parser v0.1.0\n");

    if (!otf::listFeatures_BufferStorage(args[1])) {
    // if (!otf::listFeatures_FileStorage(args[1])) {
        return 1;
    }

    return 0;
}
*/
