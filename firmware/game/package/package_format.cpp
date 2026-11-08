#include "game/package/package_format.hpp"

#include <cstring>

namespace {

uint16_t readUint16(const uint8_t* bytes)
{
    return static_cast<uint16_t>(bytes[0]) |
           (static_cast<uint16_t>(bytes[1]) << 8);
}

uint32_t readUint32(const uint8_t* bytes)
{
    return static_cast<uint32_t>(bytes[0]) |
           (static_cast<uint32_t>(bytes[1]) << 8) |
           (static_cast<uint32_t>(bytes[2]) << 16) |
           (static_cast<uint32_t>(bytes[3]) << 24);
}

} // namespace

bool decodePackageHeader(
    const uint8_t* bytes,
    size_t length,
    uint64_t packageSize,
    PackageHeader& header)
{
    header = {};
    static constexpr uint8_t MAGIC[] = {'A', 'L', 'I', 'P'};
    if (bytes == nullptr || length < ALI_HEADER_SIZE ||
        packageSize < ALI_HEADER_SIZE ||
        std::memcmp(bytes, MAGIC, sizeof(MAGIC)) != 0) {
        return false;
    }

    header.version = readUint16(bytes + 4);
    header.entryCount = readUint16(bytes + 6);
    header.indexSize = readUint32(bytes + 8);

    if (header.version != ALI_FORMAT_VERSION ||
        header.entryCount == 0 || header.entryCount > ALI_MAX_ENTRIES ||
        header.indexSize > ALI_MAX_INDEX_SIZE ||
        header.indexSize > packageSize - ALI_HEADER_SIZE) {
        header = {};
        return false;
    }

    return true;
}

bool decodePackageEntry(const uint8_t* bytes, size_t length, PackageEntry& entry)
{
    entry = {};
    if (bytes == nullptr || length < 1) {
        return false;
    }

    const size_t pathLength = bytes[0];
    if (pathLength == 0 || pathLength > ALI_MAX_PATH_LENGTH ||
        length != 1 + pathLength + 8 ||
        !isValidPackagePath(reinterpret_cast<const char*>(bytes + 1), pathLength)) {
        return false;
    }

    std::memcpy(entry.path, bytes + 1, pathLength);
    entry.path[pathLength] = '\0';
    entry.offset = readUint32(bytes + 1 + pathLength);
    entry.size = readUint32(bytes + 1 + pathLength + 4);
    return true;
}

bool isValidPackagePath(const char* path, size_t length)
{
    if (path == nullptr || length == 0 || length > ALI_MAX_PATH_LENGTH ||
        path[0] == '/' || path[length - 1] == '/') {
        return false;
    }

    size_t segmentStart = 0;
    for (size_t i = 0; i <= length; ++i) {
        if (i < length) {
            const unsigned char character = static_cast<unsigned char>(path[i]);
            if (character < 0x20 || character > 0x7E || character == '\\') {
                return false;
            }
        }

        if (i == length || path[i] == '/') {
            const size_t segmentLength = i - segmentStart;
            if (segmentLength == 0 ||
                (segmentLength == 1 && path[segmentStart] == '.') ||
                (segmentLength == 2 && path[segmentStart] == '.' &&
                 path[segmentStart + 1] == '.')) {
                return false;
            }
            segmentStart = i + 1;
        }
    }

    return true;
}

bool isPackageEntryInRange(
    const PackageEntry& entry,
    uint64_t payloadStart,
    uint64_t packageSize)
{
    const uint64_t end = static_cast<uint64_t>(entry.offset) + entry.size;
    return entry.offset >= payloadStart && end <= packageSize;
}

bool packageEntriesConflict(const PackageEntry& left, const PackageEntry& right)
{
    if (std::strcmp(left.path, right.path) == 0) {
        return true;
    }

    if (left.size == 0 || right.size == 0) {
        return false;
    }

    const uint64_t leftEnd = static_cast<uint64_t>(left.offset) + left.size;
    const uint64_t rightEnd = static_cast<uint64_t>(right.offset) + right.size;
    return left.offset < rightEnd && right.offset < leftEnd;
}
