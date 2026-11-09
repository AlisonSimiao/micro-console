#include "game/package/package_reader.hpp"

#include <cstring>
#include <limits>

namespace {

constexpr size_t MAX_SKIP_BUFFER_SIZE = 64;

bool readExact(StorageFile& file, uint8_t* buffer, size_t length)
{
    size_t read = 0;
    while (read < length) {
        const size_t count = file.read(buffer + read, length - read);
        if (count == 0) {
            return false;
        }
        read += count;
    }

    return true;
}

} // namespace

PackageFile::PackageFile() : file_(), size_(0), remaining_(0) {}

PackageFile::~PackageFile()
{
    close();
}

PackageFile::PackageFile(PackageFile&& other) noexcept
    : file_(static_cast<StorageFile&&>(other.file_)),
      size_(other.size_),
      remaining_(other.remaining_)
{
    other.size_ = 0;
    other.remaining_ = 0;
}

PackageFile& PackageFile::operator=(PackageFile&& other) noexcept
{
    if (this != &other) {
        close();
        file_ = static_cast<StorageFile&&>(other.file_);
        size_ = other.size_;
        remaining_ = other.remaining_;
        other.size_ = 0;
        other.remaining_ = 0;
    }

    return *this;
}

size_t PackageFile::read(uint8_t* buffer, size_t length)
{
    if (!isOpen() || buffer == nullptr || length == 0 || remaining_ == 0) {
        return 0;
    }

    const size_t requested = length < remaining_ ? length : remaining_;
    const size_t count = file_.read(buffer, requested);
    if (count == 0 || count > requested) {
        close();
        return 0;
    }

    remaining_ -= static_cast<uint32_t>(count);
    return count;
}

size_t PackageFile::available() const
{
    return isOpen() ? remaining_ : 0;
}

size_t PackageFile::size() const
{
    return size_;
}

bool PackageFile::isOpen() const
{
    return file_.isOpen();
}

void PackageFile::close()
{
    file_.close();
    size_ = 0;
    remaining_ = 0;
}

void PackageFile::attach(StorageFile&& file, uint32_t size)
{
    close();
    file_ = static_cast<StorageFile&&>(file);
    size_ = size;
    remaining_ = size;
}

PackageReader::PackageReader(Storage& storage) : storage_(storage) {}

bool PackageReader::readIndex(
    const char* packagePath,
    PackageEntry* entries,
    size_t capacity,
    uint8_t& entryCount,
    uint32_t& packageSize) const
{
    entryCount = 0;
    packageSize = 0;
    if (entries == nullptr || capacity == 0) {
        return false;
    }

    StorageFile file;
    if (!storage_.open(packagePath, file)) {
        return false;
    }

    const size_t fileSize = file.size();
    if (fileSize > std::numeric_limits<uint32_t>::max() || fileSize < HEADER_SIZE) {
        return false;
    }

    uint8_t header[HEADER_SIZE];
    if (!readExact(file, header, sizeof(header))) {
        return false;
    }

    PackageHeader packageHeader{};
    if (!decodePackageHeader(header, sizeof(header), fileSize, packageHeader) ||
        packageHeader.entryCount > capacity) {
        return false;
    }

    const uint64_t payloadStart = HEADER_SIZE +
                                  static_cast<uint64_t>(packageHeader.indexSize);
    size_t indexBytesRead = 0;
    for (uint16_t i = 0; i < packageHeader.entryCount; ++i) {
        uint8_t pathLength = 0;
        if (indexBytesRead >= packageHeader.indexSize ||
            !readExact(file, &pathLength, 1)) {
            return false;
        }
        ++indexBytesRead;

        const size_t recordLength = 1 + static_cast<size_t>(pathLength) + 8;
        if (pathLength == 0 || pathLength > MAX_PATH_LENGTH ||
            recordLength - 1 > packageHeader.indexSize - indexBytesRead) {
            return false;
        }
        uint8_t record[1 + MAX_PATH_LENGTH + 8];
        record[0] = pathLength;
        if (!readExact(file, record + 1, recordLength - 1)) {
            return false;
        }
        indexBytesRead += recordLength - 1;

        PackageEntry entry{};
        if (!decodePackageEntry(record, recordLength, entry) ||
            !isPackageEntryInRange(entry, payloadStart, fileSize)) {
            return false;
        }

        for (uint16_t previous = 0; previous < i; ++previous) {
            if (packageEntriesConflict(entry, entries[previous])) {
                return false;
            }
        }

        entries[i] = entry;
    }

    if (indexBytesRead != packageHeader.indexSize) {
        return false;
    }

    entryCount = static_cast<uint8_t>(packageHeader.entryCount);
    packageSize = static_cast<uint32_t>(fileSize);
    return true;
}

bool PackageReader::openFile(
    const char* packagePath,
    const PackageEntry& entry,
    uint32_t packageSize,
    PackageFile& file) const
{
    file.close();
    const uint64_t end = static_cast<uint64_t>(entry.offset) + entry.size;
    if (entry.offset > packageSize || end > packageSize) {
        return false;
    }

    StorageFile storageFile;
    if (!storage_.open(packagePath, storageFile) || storageFile.size() != packageSize) {
        return false;
    }

    uint8_t headerBytes[HEADER_SIZE];
    if (!readExact(storageFile, headerBytes, sizeof(headerBytes))) {
        return false;
    }

    PackageHeader header{};
    if (!decodePackageHeader(headerBytes, sizeof(headerBytes), packageSize, header)) {
        return false;
    }

    const uint64_t payloadStart = HEADER_SIZE + static_cast<uint64_t>(header.indexSize);
    if (!isPackageEntryInRange(entry, payloadStart, packageSize)) {
        return false;
    }

    uint32_t bytesToSkip = entry.offset - HEADER_SIZE;
    uint8_t buffer[MAX_SKIP_BUFFER_SIZE];
    while (bytesToSkip > 0) {
        const size_t chunk = bytesToSkip < sizeof(buffer) ? bytesToSkip : sizeof(buffer);
        if (!readExact(storageFile, buffer, chunk)) {
            return false;
        }
        bytesToSkip -= static_cast<uint32_t>(chunk);
    }

    file.attach(static_cast<StorageFile&&>(storageFile), entry.size);
    return true;
}

bool PackageReader::isValidPath(const char* path, size_t length)
{
    return isValidPackagePath(path, length);
}
