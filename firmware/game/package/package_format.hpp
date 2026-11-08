#pragma once

#include <cstddef>
#include <cstdint>

constexpr uint16_t ALI_FORMAT_VERSION = 1;
constexpr uint8_t ALI_MAX_ENTRIES = 64;
constexpr size_t ALI_MAX_PATH_LENGTH = 127;
constexpr size_t ALI_HEADER_SIZE = 12;
constexpr size_t ALI_MAX_INDEX_SIZE = ALI_MAX_ENTRIES * (1 + ALI_MAX_PATH_LENGTH + 8);

struct PackageEntry {
    char path[ALI_MAX_PATH_LENGTH + 1];
    uint32_t offset;
    uint32_t size;
};

struct PackageHeader {
    uint16_t version;
    uint16_t entryCount;
    uint32_t indexSize;
};

bool decodePackageHeader(
    const uint8_t* bytes,
    size_t length,
    uint64_t packageSize,
    PackageHeader& header);

bool decodePackageEntry(const uint8_t* bytes, size_t length, PackageEntry& entry);
bool isValidPackagePath(const char* path, size_t length);
bool isPackageEntryInRange(
    const PackageEntry& entry,
    uint64_t payloadStart,
    uint64_t packageSize);
bool packageEntriesConflict(const PackageEntry& left, const PackageEntry& right);
