#include "hal/storage/storage.hpp"

#include <Arduino.h>
#include <cstring>

#include "hal/storage/storage_backend.hpp"

namespace {

constexpr int DEFAULT_SCK_PIN = 5;
constexpr int DEFAULT_MISO_PIN = 35;
constexpr int DEFAULT_MOSI_PIN = 2;
constexpr int DEFAULT_CS_PIN = 15;
constexpr int DEFAULT_FREQUENCY = 20000000;
constexpr uint32_t MIN_SPI_FREQUENCY = 400000;
constexpr uint32_t MAX_SPI_FREQUENCY = 40000000;
constexpr size_t MAX_PATH_LENGTH = 128;

struct Settings {
    int sck;
    int miso;
    int mosi;
    int cs;
    int frequency;
};

Settings readSettings(const Config& config)
{
    return {
        config.getInt("storage.sck", DEFAULT_SCK_PIN),
        config.getInt("storage.miso", DEFAULT_MISO_PIN),
        config.getInt("storage.mosi", DEFAULT_MOSI_PIN),
        config.getInt("storage.cs", DEFAULT_CS_PIN),
        config.getInt("storage.frequency", DEFAULT_FREQUENCY),
    };
}

bool isValidGpioPin(int pin)
{
    return (pin >= 0 && pin <= 5) ||
           (pin >= 12 && pin <= 19) ||
           (pin >= 21 && pin <= 23) ||
           (pin >= 25 && pin <= 27) ||
           (pin >= 32 && pin <= 39);
}

bool isOutputCapable(int pin)
{
    return isValidGpioPin(pin) && pin < 34;
}

bool validateSettings(const Settings& settings)
{
    const int pins[] = {settings.sck, settings.miso, settings.mosi, settings.cs};
    const char* const configKeys[] = {
        "storage.sck",
        "storage.miso",
        "storage.mosi",
        "storage.cs",
    };

    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        if (!isValidGpioPin(pins[i])) {
            Serial.print("Storage: invalid pin for ");
            Serial.print(configKeys[i]);
            Serial.print(": ");
            Serial.println(pins[i]);
            return false;
        }

        if (i != 1 && !isOutputCapable(pins[i])) {
            Serial.print("Storage: pin requires output capability for ");
            Serial.print(configKeys[i]);
            Serial.print(": ");
            Serial.println(pins[i]);
            return false;
        }
    }

    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        for (size_t j = i + 1; j < sizeof(pins) / sizeof(pins[0]); ++j) {
            if (pins[i] == pins[j]) {
                Serial.print("Storage: pin conflict between ");
                Serial.print(configKeys[i]);
                Serial.print(" and ");
                Serial.println(configKeys[j]);
                return false;
            }
        }
    }

    if (settings.frequency < static_cast<int>(MIN_SPI_FREQUENCY) ||
        settings.frequency > static_cast<int>(MAX_SPI_FREQUENCY)) {
        Serial.print("Storage: SPI frequency out of range: ");
        Serial.println(settings.frequency);
        return false;
    }

    return true;
}

bool normalizePath(const char* path, char (&normalized)[MAX_PATH_LENGTH])
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

    if (!isAbsolute) {
        normalized[0] = '/';
    }
    memcpy(normalized + prefixLength, path, length);
    normalized[length + prefixLength] = '\0';

    const char* segment = normalized;
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

} // namespace

StorageFile::StorageFile() : backend(nullptr), slot(0) {}

StorageFile::~StorageFile()
{
    close();
}

StorageFile::StorageFile(StorageFile&& other) noexcept
    : backend(other.backend), slot(other.slot)
{
    other.backend = nullptr;
}

StorageFile& StorageFile::operator=(StorageFile&& other) noexcept
{
    if (this != &other) {
        close();
        backend = other.backend;
        slot = other.slot;
        other.backend = nullptr;
    }

    return *this;
}

void StorageFile::attach(StorageBackend& fileBackend, uint8_t fileSlot)
{
    close();
    backend = &fileBackend;
    slot = fileSlot;
}

size_t StorageFile::read(uint8_t* buffer, size_t length)
{
    if (backend == nullptr || buffer == nullptr || length == 0) {
        return 0;
    }

    return backend->read(slot, buffer, length);
}

size_t StorageFile::available() const
{
    return backend != nullptr ? backend->available(slot) : 0;
}

size_t StorageFile::size() const
{
    return backend != nullptr ? backend->size(slot) : 0;
}

bool StorageFile::isOpen() const
{
    return backend != nullptr && backend->isFileOpen(slot);
}

void StorageFile::close()
{
    if (backend != nullptr) {
        backend->close(slot);
        backend = nullptr;
    }
}

StorageDirectory::StorageDirectory() : backend(nullptr), slot(0) {}

StorageDirectory::~StorageDirectory()
{
    close();
}

StorageDirectory::StorageDirectory(StorageDirectory&& other) noexcept
    : backend(other.backend), slot(other.slot)
{
    other.backend = nullptr;
}

StorageDirectory& StorageDirectory::operator=(StorageDirectory&& other) noexcept
{
    if (this != &other) {
        close();
        backend = other.backend;
        slot = other.slot;
        other.backend = nullptr;
    }

    return *this;
}

void StorageDirectory::attach(StorageBackend& directoryBackend, uint8_t directorySlot)
{
    close();
    backend = &directoryBackend;
    slot = directorySlot;
}

bool StorageDirectory::next(StorageEntry& entry)
{
    entry.name[0] = '\0';
    entry.isDirectory = false;
    if (backend == nullptr) {
        return false;
    }

    return backend->nextEntry(slot, entry.name, sizeof(entry.name), entry.isDirectory);
}

bool StorageDirectory::isOpen() const
{
    return backend != nullptr && backend->isFileOpen(slot);
}

void StorageDirectory::close()
{
    if (backend != nullptr) {
        backend->close(slot);
        backend = nullptr;
    }
}

Storage::Storage(Config& config) : config(config), backend() {}

bool Storage::begin()
{
    setInstantiated(false);

    if (backend.isMounted()) {
        setInstantiated(true);
        return true;
    }

    const Settings settings = readSettings(config);
    if (!validateSettings(settings)) {
        Serial.println("Storage: invalid configuration");
        return false;
    }

    if (!backend.begin(
            settings.sck,
            settings.miso,
            settings.mosi,
            settings.cs,
            static_cast<uint32_t>(settings.frequency))) {
        Serial.println("Storage: card initialization or mount failed");
        return false;
    }

    setInstantiated(true);
    Serial.println("Storage: card mounted");
    return true;
}

bool Storage::claimPins(PinClaims& claims) const
{
    const Settings settings = readSettings(config);
    const int pins[] = {settings.sck, settings.miso, settings.mosi, settings.cs};
    const char* const configKeys[] = {
        "storage.sck",
        "storage.miso",
        "storage.mosi",
        "storage.cs",
    };

    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        if (pins[i] >= 0 && !claims.add(pins[i], configKeys[i])) {
            return false;
        }
    }

    return true;
}

bool Storage::isMounted() const
{
    return isInstantiated() && backend.isMounted();
}

bool Storage::exists(const char* path) const
{
    char normalized[MAX_PATH_LENGTH];
    return normalizePath(path, normalized) && isMounted() && backend.exists(normalized);
}

bool Storage::isDirectory(const char* path) const
{
    char normalized[MAX_PATH_LENGTH];
    return normalizePath(path, normalized) && isMounted() && backend.isDirectory(normalized);
}

bool Storage::open(const char* path, StorageFile& file) const
{
    file.close();

    char normalized[MAX_PATH_LENGTH];
    if (!normalizePath(path, normalized) || !isMounted()) {
        return false;
    }

    uint8_t slot = 0;
    if (!backend.open(normalized, slot)) {
        return false;
    }

    file.attach(backend, slot);
    return true;
}

bool Storage::openDirectory(const char* path, StorageDirectory& directory) const
{
    directory.close();

    char normalized[MAX_PATH_LENGTH];
    if (!normalizePath(path, normalized) || !isMounted()) {
        return false;
    }

    uint8_t slot = 0;
    if (!backend.openDirectory(normalized, slot)) {
        return false;
    }

    directory.attach(backend, slot);
    return true;
}

bool Storage::getInfo(const char* path, StorageFileInfo& info) const
{
    info = {false, 0};

    char normalized[MAX_PATH_LENGTH];
    if (!normalizePath(path, normalized) || !isMounted()) {
        return false;
    }

    return backend.getInfo(normalized, info.isDirectory, info.size);
}
