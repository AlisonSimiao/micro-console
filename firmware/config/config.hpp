#pragma once

#include <cstddef>

class Config {
public:
    static constexpr size_t MAX_KEY_LENGTH = 32;
    static constexpr size_t MAX_VALUE_LENGTH = 64;
    static constexpr size_t MAX_ENTRIES = 32;

    Config();

    bool begin(const char* path = "/config.ini");

    int getInt(const char* key, int defaultValue = 0) const;
    const char* getString(const char* key, const char* defaultValue = "") const;

    bool setInt(const char* key, int value);
    bool setString(const char* key, const char* value);

private:
    struct Entry {
        char key[MAX_KEY_LENGTH];
        char value[MAX_VALUE_LENGTH];
    };

    Entry entries[MAX_ENTRIES];
    size_t entryCount;

    const Entry* find(const char* key) const;
    Entry* find(const char* key);
    bool set(const char* key, const char* value);
    bool loadFile(const char* path);
    void parseLine(char* line);
};
