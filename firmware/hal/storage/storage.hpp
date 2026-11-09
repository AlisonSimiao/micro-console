#pragma once

#include <cstddef>
#include <cstdint>

#include "config/config.hpp"
#include "hal/core/hal_module.hpp"
#include "hal/storage/storage_backend.hpp"

struct StorageFileInfo {
    bool isDirectory;
    size_t size;
};

struct StorageEntry {
    char name[128];
    bool isDirectory;
};

class StorageFile {
public:
    StorageFile();
    ~StorageFile();

    StorageFile(const StorageFile&) = delete;
    StorageFile& operator=(const StorageFile&) = delete;

    StorageFile(StorageFile&& other) noexcept;
    StorageFile& operator=(StorageFile&& other) noexcept;

    size_t read(uint8_t* buffer, size_t length);
    size_t available() const;
    size_t size() const;
    bool isOpen() const;
    void close();

private:
    friend class Storage;

    void attach(StorageBackend& backend, uint8_t slot);

    StorageBackend* backend;
    uint8_t slot;
};

class StorageDirectory {
public:
    StorageDirectory();
    ~StorageDirectory();

    StorageDirectory(const StorageDirectory&) = delete;
    StorageDirectory& operator=(const StorageDirectory&) = delete;

    StorageDirectory(StorageDirectory&& other) noexcept;
    StorageDirectory& operator=(StorageDirectory&& other) noexcept;

    bool next(StorageEntry& entry);
    bool isOpen() const;
    void close();

private:
    friend class Storage;

    void attach(StorageBackend& backend, uint8_t slot);

    StorageBackend* backend;
    uint8_t slot;
};

class Storage : public hal_module {
public:
    explicit Storage(Config& config);

    bool begin() override;
    bool claimPins(PinClaims& claims) const override;

    bool isMounted() const;
    bool exists(const char* path) const;
    bool isDirectory(const char* path) const;
    bool open(const char* path, StorageFile& file) const;
    bool openDirectory(const char* path, StorageDirectory& directory) const;
    bool getInfo(const char* path, StorageFileInfo& info) const;

private:
    Config& config;
    // A API pública consulta o Storage em métodos const, mas `open()` aloca um
    // slot no backend. `mutable` preserva a assinatura const da API.
    mutable StorageBackend backend;
};
