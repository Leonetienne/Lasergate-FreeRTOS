#ifndef LASERGATE_V2_NVSESP32_H
#define LASERGATE_V2_NVSESP32_H

#include "hal/INVS.h"
#include "nvs.h"

/**
 * Esp32-Implementation for non-volatile key/value storage
 */
class NVSEsp32 : public INVS {
public:
    NVSEsp32() = default;
    NVSEsp32(const NVSEsp32&) = delete;
    NVSEsp32& operator=(const NVSEsp32&) = delete;
    NVSEsp32(NVSEsp32&&) noexcept;
    NVSEsp32& operator=(NVSEsp32&&) = delete;
    ~NVSEsp32() noexcept override;

    /**
     * Opens the given namespace for reading and writing
     * @param namespaceName
     * @return Success state
     */
    bool begin(const char* namespaceName) noexcept override;

    /**
     * Will release the resources acquired by begin()
     * @return Success state
     */
    bool free() noexcept override;

    /**
     * Stores an integer value under the given key
     * @param key
     * @param value
     * @return Success state
     */
    bool setInt(const char* key, int32_t value) noexcept override;

    /**
     * Reads the integer value stored under the given key
     * @param key
     * @param outValue receives the stored value, left untouched on failure
     * @return Success state
     */
    bool getInt(const char* key, int32_t& outValue) const noexcept override;

    /**
     * Stores a string value (at most NVS_MAX_STRING_LENGTH characters) under the given key
     * @param key
     * @param value
     * @return Success state
     */
    bool setString(const char* key, const char* value) noexcept override;

    /**
     * Reads the string value stored under the given key
     * @param key
     * @param outValue buffer of at least NVS_MAX_STRING_LENGTH + 1 bytes to receive the stored value
     * @return Success state
     */
    bool getString(const char* key, char* outValue) const noexcept override;

    /**
     * Erases the value stored under the given key, if any. A key that was never set is not a failure.
     * @param key
     * @return Success state
     */
    bool eraseKey(const char* key) noexcept override;

private:
    nvs_handle_t handle = 0;
};

#endif //LASERGATE_V2_NVSESP32_H
