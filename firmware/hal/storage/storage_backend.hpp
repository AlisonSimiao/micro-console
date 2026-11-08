#pragma once

#include <cstddef>
#include <cstdint>

#include <SD.h>

class StorageBackend {
public:
    static constexpr uint8_t MAX_OPEN_FILES = 4;

    StorageBackend();

    bool begin(int sck, int miso, int mosi, int cs, uint32_t frequency);
    bool isMounted() const;
    bool exists(const char* path) const;
    bool isDirectory(const char* path) const;
    bool open(const char* path, uint8_t& slot);
    bool getInfo(const char* path, bool& isDirectory, size_t& size) const;

    size_t read(uint8_t slot, uint8_t* buffer, size_t length);
    size_t available(uint8_t slot);
    size_t size(uint8_t slot) const;
    bool isFileOpen(uint8_t slot) const;
    void close(uint8_t slot);

private:
    struct OpenFile {
        File file;
        bool inUse;
    };

    SPIClass spi;
    OpenFile files[MAX_OPEN_FILES];
    bool spiStarted;
    bool mounted;
};
