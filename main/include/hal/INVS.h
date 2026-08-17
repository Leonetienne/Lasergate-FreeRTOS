#ifndef LASERGATE_V2_INVS_H
#define LASERGATE_V2_INVS_H

#include <cstdint>
#include <cstddef>

constexpr std::size_t NVS_MAX_STRING_LENGTH = 64;

/**
 * Abstract interface for non-volatile key/value storage
 */
class INVS {
public:
    INVS() = default;
    INVS(const INVS&) = delete;
    INVS& operator=(const INVS&) = delete;
    INVS& operator=(INVS&&) = delete;
    virtual ~INVS() = default;

    /**
     * Opens the given namespace for reading and writing
     * @param namespaceName
     * @return Success state
     */
    virtual bool begin(const char* namespaceName) noexcept = 0;

    /**
     * Will release the resources acquired by begin()
     * @return Success state
     */
    virtual bool free() noexcept = 0;

    /**
     * Stores an integer value under the given key
     * @param key
     * @param value
     * @return Success state
     */
    virtual bool setInt(const char* key, int32_t value) noexcept = 0;

    /**
     * Reads the integer value stored under the given key
     * @param key
     * @param outValue receives the stored value, left untouched on failure
     * @return Success state
     */
    [[nodiscard]] virtual bool getInt(const char* key, int32_t& outValue) const noexcept = 0;

    /**
     * Stores a string value (at most NVS_MAX_STRING_LENGTH characters) under the given key
     * @param key
     * @param value
     * @return Success state
     */
    virtual bool setString(const char* key, const char* value) noexcept = 0;

    /**
     * Reads the string value stored under the given key
     * @param key
     * @param outValue buffer of at least NVS_MAX_STRING_LENGTH + 1 bytes to receive the stored value
     * @return Success state
     */
    [[nodiscard]] virtual bool getString(const char* key, char* outValue) const noexcept = 0;

    /**
     * Erases the value stored under the given key, if any. A key that was never set is not a failure.
     * @param key
     * @return Success state
     */
    virtual bool eraseKey(const char* key) noexcept = 0;

    /**
     * @return Whether the nvs system is ready and initialized
     */
    [[nodiscard]] bool isReady() const noexcept { return isInitialized; }

protected:
    INVS(INVS&& other) noexcept : isInitialized(other.isInitialized) {
        other.isInitialized = false;
    }

    bool isInitialized = false;
};

#endif //LASERGATE_V2_INVS_H
