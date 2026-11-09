#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <utility>
#include <vector>

struct StorageFileInfo {
    bool isDirectory;
    size_t size;
};

class Storage;

namespace test_storage {

constexpr size_t MAX_PATH_LENGTH = 128;
constexpr size_t MAX_OPEN_FILES = 4;

// Reproduz as regras de normalizePath() em firmware/hal/storage/storage.cpp:
// caminhos relativos recebem '/', caminhos com '..' são recusados e o caminho
// normalizado precisa caber em MAX_PATH_LENGTH - 1 bytes.
inline bool normalizePath(const char* path, std::string& normalized)
{
    if (path == nullptr || path[0] == '\0') {
        return false;
    }

    size_t length = 0;
    while (length < MAX_PATH_LENGTH && path[length] != '\0') {
        ++length;
    }
    if (length == MAX_PATH_LENGTH) {
        return false;
    }

    const bool isAbsolute = path[0] == '/';
    const size_t prefixLength = isAbsolute ? 0 : 1;
    if (length + prefixLength >= MAX_PATH_LENGTH) {
        return false;
    }

    normalized.clear();
    if (!isAbsolute) {
        normalized.push_back('/');
    }
    normalized.append(path, length);

    const char* segment = normalized.c_str();
    while (*segment != '\0') {
        while (*segment == '/') {
            ++segment;
        }

        const char* end = segment;
        while (*end != '\0' && *end != '/') {
            ++end;
        }

        if (end - segment == 2 && segment[0] == '.' && segment[1] == '.') {
            return false;
        }
        segment = end;
    }

    return true;
}

} // namespace test_storage

class StorageFile {
public:
    StorageFile() : owner_(nullptr), path_(), bytes_(), position_(0) {}
    ~StorageFile() { close(); }

    StorageFile(const StorageFile&) = delete;
    StorageFile& operator=(const StorageFile&) = delete;

    StorageFile(StorageFile&& other) noexcept
        : owner_(other.owner_),
          path_(std::move(other.path_)),
          bytes_(std::move(other.bytes_)),
          position_(other.position_)
    {
        other.owner_ = nullptr;
        other.position_ = 0;
    }

    StorageFile& operator=(StorageFile&& other) noexcept
    {
        if (this != &other) {
            close();
            owner_ = other.owner_;
            path_ = std::move(other.path_);
            bytes_ = std::move(other.bytes_);
            position_ = other.position_;
            other.owner_ = nullptr;
            other.position_ = 0;
        }
        return *this;
    }

    size_t read(uint8_t* buffer, size_t length);
    size_t available() const;
    size_t size() const { return isOpen() ? bytes_.size() : 0; }
    bool isOpen() const { return owner_ != nullptr; }
    void close();

private:
    friend class Storage;

    void attach(Storage& storage, const std::string& path, const std::vector<uint8_t>& bytes);

    Storage* owner_;
    std::string path_;
    std::vector<uint8_t> bytes_;
    size_t position_;
};

class Storage {
public:
    Storage() : mounted_(true), maxReadChunk_(static_cast<size_t>(-1)), openFileCount_(0), bytesRead_(0) {}

    bool isMounted() const { return mounted_; }
    void setMounted(bool mounted) { mounted_ = mounted; }

    bool exists(const char* path) const
    {
        std::string key;
        return mounted_ && test_storage::normalizePath(path, key) &&
               files_.find(key) != files_.end();
    }

    bool isDirectory(const char* path) const
    {
        (void)path;
        return false;
    }

    bool getInfo(const char* path, StorageFileInfo& info) const
    {
        info = {false, 0};
        std::string key;
        if (!mounted_ || !test_storage::normalizePath(path, key)) {
            return false;
        }
        const std::map<std::string, std::vector<uint8_t> >::const_iterator found =
            files_.find(key);
        if (found == files_.end()) {
            return false;
        }
        info.size = found->second.size();
        return true;
    }

    bool open(const char* path, StorageFile& file) const
    {
        file.close();
        std::string key;
        if (!mounted_ || !test_storage::normalizePath(path, key)) {
            return false;
        }
        const std::map<std::string, std::vector<uint8_t> >::const_iterator found =
            files_.find(key);
        if (found == files_.end() || openFileCount_ >= test_storage::MAX_OPEN_FILES) {
            return false;
        }
        file.attach(*const_cast<Storage*>(this), found->first, found->second);
        return true;
    }

    void addFile(const std::string& path, const std::vector<uint8_t>& bytes)
    {
        std::string key;
        if (test_storage::normalizePath(path.c_str(), key)) {
            files_[key] = bytes;
        }
    }

    void replaceFile(const std::string& path, const std::vector<uint8_t>& bytes)
    {
        addFile(path, bytes);
    }

    void setMaxReadChunk(size_t size) { maxReadChunk_ = size; }

    void setFailAt(const std::string& path, size_t offset)
    {
        std::string key;
        if (test_storage::normalizePath(path.c_str(), key)) {
            failAt_[key] = offset;
        }
    }

    size_t openFileCount() const { return openFileCount_; }
    size_t bytesRead() const { return bytesRead_; }

private:
    friend class StorageFile;

    size_t read(const std::string& path, const std::vector<uint8_t>& bytes,
                size_t position, uint8_t* buffer, size_t length) const
    {
        if (!mounted_ || buffer == nullptr || length == 0 || position >= bytes.size()) {
            return 0;
        }

        size_t count = std::min(length, bytes.size() - position);
        count = std::min(count, maxReadChunk_);
        const std::map<std::string, size_t>::const_iterator fail = failAt_.find(path);
        if (fail != failAt_.end()) {
            if (position >= fail->second) {
                return 0;
            }
            count = std::min(count, fail->second - position);
        }
        if (count == 0) {
            return 0;
        }

        std::memcpy(buffer, bytes.data() + position, count);
        bytesRead_ += count;
        return count;
    }

    void fileOpened() { ++openFileCount_; }
    void fileClosed()
    {
        if (openFileCount_ > 0) {
            --openFileCount_;
        }
    }

    bool mounted_;
    size_t maxReadChunk_;
    mutable size_t openFileCount_;
    mutable size_t bytesRead_;
    std::map<std::string, std::vector<uint8_t> > files_;
    std::map<std::string, size_t> failAt_;
};

inline void StorageFile::attach(
    Storage& storage,
    const std::string& path,
    const std::vector<uint8_t>& bytes)
{
    close();
    owner_ = &storage;
    path_ = path;
    bytes_ = bytes;
    position_ = 0;
    owner_->fileOpened();
}

inline size_t StorageFile::read(uint8_t* buffer, size_t length)
{
    if (owner_ == nullptr) {
        return 0;
    }
    const size_t count = owner_->read(path_, bytes_, position_, buffer, length);
    position_ += count;
    return count;
}

inline size_t StorageFile::available() const
{
    return isOpen() && position_ < bytes_.size() ? bytes_.size() - position_ : 0;
}

inline void StorageFile::close()
{
    if (owner_ != nullptr) {
        owner_->fileClosed();
        owner_ = nullptr;
        path_.clear();
        bytes_.clear();
        position_ = 0;
    }
}
