#include "game/package/game_manifest.hpp"

#include <ArduinoJson.h>
#include <cstring>

#include "game/package/package_format.hpp"

namespace {

bool copyString(
    JsonObjectConst object,
    const char* key,
    char* destination,
    size_t capacity,
    size_t minimumLength,
    size_t maximumLength)
{
    const JsonVariantConst value = object[key];
    if (!value.is<const char*>()) {
        return false;
    }

    const JsonString text = value.as<JsonString>();
    const size_t length = text.size();
    if (text.isNull() || length < minimumLength || length > maximumLength ||
        length >= capacity || std::memchr(text.c_str(), '\0', length) != nullptr) {
        return false;
    }

    std::memcpy(destination, text.c_str(), length);
    destination[length] = '\0';
    return true;
}

bool isValidId(const char* id)
{
    for (const char* character = id; *character != '\0'; ++character) {
        if (!((*character >= 'a' && *character <= 'z') ||
              (*character >= '0' && *character <= '9') ||
              *character == '.' || *character == '_' || *character == '-')) {
            return false;
        }
    }
    return true;
}

} // namespace

bool parseGameManifest(const char* json, size_t length, GameManifest& manifest)
{
    manifest = {};
    if (json == nullptr || length == 0 || length > GAME_MANIFEST_MAX_SIZE ||
        std::memchr(json, '\0', length) != nullptr) {
        return false;
    }

    JsonDocument document;
    const DeserializationError error = deserializeJson(
        document,
        json,
        length,
        DeserializationOption::NestingLimit(8));
    if (error || !document.is<JsonObjectConst>()) {
        return false;
    }

    const JsonObjectConst object = document.as<JsonObjectConst>();
    const JsonVariantConst packageVersion = object["packageVersion"];
    if (!packageVersion.is<uint16_t>() ||
        packageVersion.as<uint16_t>() != ALI_FORMAT_VERSION ||
        !copyString(object, "id", manifest.id, sizeof(manifest.id), 1, 63) ||
        !isValidId(manifest.id) ||
        !copyString(object, "name", manifest.name, sizeof(manifest.name), 1, 63) ||
        !copyString(object, "version", manifest.version, sizeof(manifest.version), 1, 31) ||
        !copyString(object, "author", manifest.author, sizeof(manifest.author), 1, 63) ||
        !copyString(
            object,
            "entrypoint",
            manifest.entrypoint,
            sizeof(manifest.entrypoint),
            1,
            ALI_MAX_PATH_LENGTH) ||
        !isValidPackagePath(manifest.entrypoint, std::strlen(manifest.entrypoint)) ||
        !copyString(
            object,
            "cover",
            manifest.cover,
            sizeof(manifest.cover),
            1,
            ALI_MAX_PATH_LENGTH) ||
        !isValidPackagePath(manifest.cover, std::strlen(manifest.cover))) {
        manifest = {};
        return false;
    }

    manifest.packageVersion = ALI_FORMAT_VERSION;
    return true;
}
