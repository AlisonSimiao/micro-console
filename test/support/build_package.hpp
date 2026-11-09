#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "game/package/package_format.hpp"

struct TestEntry {
    std::string path;
    std::vector<uint8_t> data;
    bool overrideOffset;
    uint32_t offset;
    bool overrideSize;
    uint32_t size;

    TestEntry(const std::string& path, const std::vector<uint8_t>& data)
        : path(path), data(data), overrideOffset(false), offset(0),
          overrideSize(false), size(0)
    {
    }
};

inline void appendUint16(std::vector<uint8_t>& bytes, uint16_t value)
{
    bytes.push_back(static_cast<uint8_t>(value));
    bytes.push_back(static_cast<uint8_t>(value >> 8));
}

inline void appendUint32(std::vector<uint8_t>& bytes, uint32_t value)
{
    bytes.push_back(static_cast<uint8_t>(value));
    bytes.push_back(static_cast<uint8_t>(value >> 8));
    bytes.push_back(static_cast<uint8_t>(value >> 16));
    bytes.push_back(static_cast<uint8_t>(value >> 24));
}

inline std::vector<uint8_t> buildPackage(
    const std::vector<TestEntry>& entries,
    uint16_t version = ALI_FORMAT_VERSION,
    bool overrideCount = false,
    uint16_t count = 0,
    bool overrideIndexSize = false,
    uint32_t indexSize = 0,
    bool overrideMagic = false)
{
    size_t calculatedIndexSize = 0;
    for (size_t i = 0; i < entries.size(); ++i) {
        calculatedIndexSize += 1 + entries[i].path.size() + 8;
    }

    const uint32_t actualIndexSize =
        overrideIndexSize ? indexSize : static_cast<uint32_t>(calculatedIndexSize);
    uint64_t nextOffset = ALI_HEADER_SIZE + calculatedIndexSize;
    std::vector<uint8_t> index;
    std::vector<uint8_t> payload;
    for (size_t i = 0; i < entries.size(); ++i) {
        const TestEntry& entry = entries[i];
        index.push_back(static_cast<uint8_t>(entry.path.size()));
        index.insert(index.end(), entry.path.begin(), entry.path.end());
        const uint32_t offset = entry.overrideOffset
                                    ? entry.offset
                                    : static_cast<uint32_t>(nextOffset);
        const uint32_t size = entry.overrideSize
                                  ? entry.size
                                  : static_cast<uint32_t>(entry.data.size());
        appendUint32(index, offset);
        appendUint32(index, size);
        payload.insert(payload.end(), entry.data.begin(), entry.data.end());
        nextOffset += entry.data.size();
    }

    std::vector<uint8_t> bytes;
    if (overrideMagic) {
        bytes.insert(bytes.end(), {'B', 'A', 'D', '!'});
    } else {
        bytes.insert(bytes.end(), {'A', 'L', 'I', 'P'});
    }
    appendUint16(bytes, version);
    appendUint16(bytes, overrideCount ? count : static_cast<uint16_t>(entries.size()));
    appendUint32(bytes, actualIndexSize);
    bytes.insert(bytes.end(), index.begin(), index.end());
    bytes.insert(bytes.end(), payload.begin(), payload.end());
    return bytes;
}

inline std::vector<uint8_t> byteVector(const std::string& value)
{
    return std::vector<uint8_t>(value.begin(), value.end());
}

inline std::vector<TestEntry> validPackageEntries()
{
    const std::string manifest =
        "{\"id\":\"test-game\",\"name\":\"Test\",\"version\":\"1\","
        "\"packageVersion\":1,\"author\":\"Test\","
        "\"entrypoint\":\"Codes/main.js\",\"cover\":\"cover.bin\"}";
    std::vector<uint8_t> large(16 * 1024);
    for (size_t i = 0; i < large.size(); ++i) {
        large[i] = static_cast<uint8_t>(i % 256);
    }
    std::vector<TestEntry> entries;
    entries.push_back(TestEntry("manifest.json", byteVector(manifest)));
    entries.push_back(TestEntry("Codes/main.js", byteVector("main-content")));
    entries.push_back(TestEntry("cover.bin", byteVector("cover")));
    entries.push_back(TestEntry("Assets/large.bin", large));
    entries.push_back(TestEntry("empty.bin", std::vector<uint8_t>()));
    return entries;
}
