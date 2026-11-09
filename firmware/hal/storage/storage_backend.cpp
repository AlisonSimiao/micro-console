#include "hal/storage/storage_backend.hpp"

#include <Arduino.h>
#include <cstring>

StorageBackend::StorageBackend()
    : spi(VSPI), files{}, spiStarted(false), mounted(false)
{
}

bool StorageBackend::begin(int sck, int miso, int mosi, int cs, uint32_t frequency)
{
    if (mounted) {
        return true;
    }

    if (!spiStarted) {
        if (!spi.begin(sck, miso, mosi, cs)) {
            Serial.println("Storage: failed to initialize SPI bus");
            return false;
        }
        spiStarted = true;
    }

    mounted = SD.begin(
        static_cast<uint8_t>(cs),
        spi,
        frequency,
        "/sd",
        MAX_OPEN_FILES,
        false);
    if (!mounted) {
        Serial.println("Storage: SD.begin failed");
    }

    return mounted;
}

bool StorageBackend::isMounted() const
{
    return mounted;
}

bool StorageBackend::exists(const char* path) const
{
    return mounted && SD.exists(path);
}

bool StorageBackend::isDirectory(const char* path) const
{
    if (!mounted) {
        return false;
    }

    File file = SD.open(path, FILE_READ);
    if (!file) {
        return false;
    }

    const bool result = file.isDirectory();
    file.close();
    return result;
}

bool StorageBackend::open(const char* path, uint8_t& slot)
{
    if (!mounted) {
        return false;
    }

    for (uint8_t i = 0; i < MAX_OPEN_FILES; ++i) {
        if (files[i].inUse) {
            continue;
        }

        File file = SD.open(path, FILE_READ);
        if (!file) {
            return false;
        }

        if (file.isDirectory()) {
            file.close();
            return false;
        }

        files[i].file = file;
        files[i].inUse = true;
        slot = i;
        return true;
    }

    Serial.println("Storage: maximum number of open files reached");
    return false;
}

bool StorageBackend::openDirectory(const char* path, uint8_t& slot)
{
    if (!mounted) {
        return false;
    }

    for (uint8_t i = 0; i < MAX_OPEN_FILES; ++i) {
        if (files[i].inUse) {
            continue;
        }

        File file = SD.open(path, FILE_READ);
        if (!file) {
            return false;
        }

        if (!file.isDirectory()) {
            file.close();
            return false;
        }

        files[i].file = file;
        files[i].inUse = true;
        slot = i;
        return true;
    }

    Serial.println("Storage: maximum number of open files reached");
    return false;
}

bool StorageBackend::getInfo(const char* path, bool& isDirectory, size_t& size) const
{
    if (!mounted) {
        return false;
    }

    File file = SD.open(path, FILE_READ);
    if (!file) {
        return false;
    }

    isDirectory = file.isDirectory();
    size = isDirectory ? 0 : file.size();
    file.close();
    return true;
}

size_t StorageBackend::read(uint8_t slot, uint8_t* buffer, size_t length)
{
    if (!isFileOpen(slot) || buffer == nullptr || length == 0) {
        return 0;
    }

    return files[slot].file.read(buffer, length);
}

size_t StorageBackend::available(uint8_t slot)
{
    if (!isFileOpen(slot)) {
        return 0;
    }

    const int count = files[slot].file.available();
    return count > 0 ? static_cast<size_t>(count) : 0;
}

size_t StorageBackend::size(uint8_t slot) const
{
    return isFileOpen(slot) ? files[slot].file.size() : 0;
}

bool StorageBackend::isFileOpen(uint8_t slot) const
{
    return slot < MAX_OPEN_FILES && files[slot].inUse && static_cast<bool>(files[slot].file);
}

bool StorageBackend::nextEntry(uint8_t slot, char* name, size_t capacity, bool& isDirectory)
{
    if (!isFileOpen(slot) || name == nullptr || capacity == 0) {
        return false;
    }

    for (;;) {
        File entry = files[slot].file.openNextFile();
        if (!entry) {
            return false;
        }

        const char* const entryName = entry.name();
        const bool entryIsDirectory = entry.isDirectory();
        bool copied = false;
        if (entryName != nullptr) {
            const size_t length = std::strlen(entryName);
            if (length > 0 && length < capacity &&
                std::strcmp(entryName, ".") != 0 && std::strcmp(entryName, "..") != 0) {
                std::memcpy(name, entryName, length + 1);
                isDirectory = entryIsDirectory;
                copied = true;
            }
        }

        entry.close();
        if (copied) {
            return true;
        }
    }
}

void StorageBackend::close(uint8_t slot)
{
    if (slot >= MAX_OPEN_FILES || !files[slot].inUse) {
        return;
    }

    files[slot].file.close();
    files[slot].file = File();
    files[slot].inUse = false;
}
