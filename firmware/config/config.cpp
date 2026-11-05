#include "config/config.hpp"

#include <Arduino.h>
#include <LittleFS.h>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

char* trim(char* text)
{
    while (*text != '\0' && isspace(static_cast<unsigned char>(*text))) {
        ++text;
    }

    char* end = text + strlen(text);
    while (end > text && isspace(static_cast<unsigned char>(*(end - 1)))) {
        --end;
    }

    *end = '\0';
    return text;
}

} // namespace

Config::Config() : entries{}, entryCount(0) {}

bool Config::begin(const char* path)
{
    entryCount = 0;
    memset(entries, 0, sizeof(entries));

    if (!LittleFS.begin(true)) {
        Serial.println("Config: failed to mount LittleFS");
        return false;
    }

    if (!loadFile(path)) {
        Serial.print("Config: file not found, using defaults: ");
        Serial.println(path != nullptr ? path : "(null)");
        return true;
    }

    Serial.print("Config: loaded ");
    Serial.print(static_cast<unsigned int>(entryCount));
    Serial.print(" entries from ");
    Serial.println(path);
    return true;
}

int Config::getInt(const char* key, int defaultValue) const
{
    const Entry* entry = find(key);
    if (entry == nullptr) {
        return defaultValue;
    }

    char* end = nullptr;
    const long value = strtol(entry->value, &end, 10);
    if (end == entry->value || *end != '\0') {
        return defaultValue;
    }

    return static_cast<int>(value);
}

const char* Config::getString(const char* key, const char* defaultValue) const
{
    const Entry* entry = find(key);
    return entry != nullptr ? entry->value : defaultValue;
}

bool Config::setInt(const char* key, int value)
{
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d", value);
    return set(key, buffer);
}

bool Config::setString(const char* key, const char* value)
{
    return set(key, value);
}

const Config::Entry* Config::find(const char* key) const
{
    if (key == nullptr) {
        return nullptr;
    }

    for (size_t i = 0; i < entryCount; ++i) {
        if (strcmp(entries[i].key, key) == 0) {
            return &entries[i];
        }
    }

    return nullptr;
}

Config::Entry* Config::find(const char* key)
{
    if (key == nullptr) {
        return nullptr;
    }

    for (size_t i = 0; i < entryCount; ++i) {
        if (strcmp(entries[i].key, key) == 0) {
            return &entries[i];
        }
    }

    return nullptr;
}

bool Config::set(const char* key, const char* value)
{
    if (key == nullptr || value == nullptr) {
        return false;
    }

    const size_t keyLength = strlen(key);
    const size_t valueLength = strlen(value);
    if (keyLength == 0 || keyLength >= MAX_KEY_LENGTH || valueLength >= MAX_VALUE_LENGTH) {
        return false;
    }

    Entry* entry = find(key);
    if (entry == nullptr) {
        if (entryCount >= MAX_ENTRIES) {
            Serial.println("Config: maximum number of entries reached");
            return false;
        }

        entry = &entries[entryCount++];
        strncpy(entry->key, key, MAX_KEY_LENGTH - 1);
        entry->key[MAX_KEY_LENGTH - 1] = '\0';
    }

    strncpy(entry->value, value, MAX_VALUE_LENGTH - 1);
    entry->value[MAX_VALUE_LENGTH - 1] = '\0';
    return true;
}

bool Config::loadFile(const char* path)
{
    if (path == nullptr) {
        return false;
    }

    File file = LittleFS.open(path, "r");
    if (!file) {
        return false;
    }

    char line[MAX_KEY_LENGTH + MAX_VALUE_LENGTH + 2];
    size_t length = 0;

    while (file.available()) {
        const int character = file.read();
        if (character < 0) {
            break;
        }

        if (character == '\n') {
            line[length] = '\0';
            parseLine(line);
            length = 0;
        } else if (character != '\r' && length < sizeof(line) - 1) {
            line[length++] = static_cast<char>(character);
        }
    }

    if (length > 0) {
        line[length] = '\0';
        parseLine(line);
    }

    file.close();
    return true;
}

void Config::parseLine(char* line)
{
    char* text = trim(line);
    if (*text == '\0' || *text == '#' || *text == ';') {
        return;
    }

    char* separator = strchr(text, '=');
    if (separator == nullptr) {
        return;
    }

    *separator = '\0';
    char* key = trim(text);
    char* value = trim(separator + 1);

    if (!set(key, value)) {
        Serial.print("Config: invalid entry: ");
        Serial.println(key);
    }
}
