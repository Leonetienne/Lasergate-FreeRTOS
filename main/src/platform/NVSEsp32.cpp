#include "platform/NVSEsp32.h"
#include "nvs_flash.h"
#include <cstring>
#include <utility>

NVSEsp32::NVSEsp32(NVSEsp32&& other) noexcept :
    INVS(std::move(other)),
    handle(other.handle)
{
    other.handle = 0;
}

NVSEsp32::~NVSEsp32() noexcept {
    if (isInitialized) {
        NVSEsp32::free();
    }
}

bool NVSEsp32::begin(const char* namespaceName) noexcept {
    if (isInitialized) {
        return false;
    }

    esp_err_t nvsResult = nvs_flash_init();
    if (nvsResult == ESP_ERR_NVS_NO_FREE_PAGES || nvsResult == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        if (nvs_flash_erase() != ESP_OK) {
            return false;
        }
        nvsResult = nvs_flash_init();
    }
    if (nvsResult != ESP_OK) {
        return false;
    }

    if (nvs_open(namespaceName, NVS_READWRITE, &handle) != ESP_OK) {
        return false;
    }

    isInitialized = true;
    return true;
}

bool NVSEsp32::free() noexcept {
    if (!isInitialized) {
        return false;
    }

    nvs_close(handle);
    isInitialized = false;
    return true;
}

bool NVSEsp32::setInt(const char* key, int32_t value) noexcept {
    if (!isInitialized) {
        return false;
    }

    if (nvs_set_i32(handle, key, value) != ESP_OK) {
        return false;
    }

    return nvs_commit(handle) == ESP_OK;
}

bool NVSEsp32::getInt(const char* key, int32_t& outValue) const noexcept {
    if (!isInitialized) {
        return false;
    }

    return nvs_get_i32(handle, key, &outValue) == ESP_OK;
}

bool NVSEsp32::setString(const char* key, const char* value) noexcept {
    if (!isInitialized || std::strlen(value) > NVS_MAX_STRING_LENGTH) {
        return false;
    }

    if (nvs_set_str(handle, key, value) != ESP_OK) {
        return false;
    }

    return nvs_commit(handle) == ESP_OK;
}

bool NVSEsp32::getString(const char* key, char* outValue) const noexcept {
    if (!isInitialized) {
        return false;
    }

    size_t length = NVS_MAX_STRING_LENGTH + 1;
    return nvs_get_str(handle, key, outValue, &length) == ESP_OK;
}

bool NVSEsp32::eraseKey(const char* key) noexcept {
    if (!isInitialized) {
        return false;
    }

    const esp_err_t result = nvs_erase_key(handle, key);
    if (result != ESP_OK && result != ESP_ERR_NVS_NOT_FOUND) {
        return false;
    }

    return nvs_commit(handle) == ESP_OK;
}
