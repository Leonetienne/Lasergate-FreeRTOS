#ifndef LASERGATE_V2_JSONWRITER_H
#define LASERGATE_V2_JSONWRITER_H

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

/**
 * Formats JSON scalars for hand-assembled API payloads.
 * Callers compose objects and arrays by string concatenation.
 */
class JsonWriter {
public:
    /**
     * @return s with '"', '\\' and control characters escaped, without surrounding quotes
     */
    [[nodiscard]] static std::string escape(std::string_view s) noexcept;

    /**
     * @return s as a quoted, escaped JSON string literal
     */
    [[nodiscard]] static std::string string(std::string_view s) noexcept;

    [[nodiscard]] static const char* boolean(bool v) noexcept;
    [[nodiscard]] static std::string number(int64_t v) noexcept;
    [[nodiscard]] static const char* null() noexcept;

    /**
     * @return "null" if v is empty, otherwise v's value formatted as a JSON number
     */
    template<typename T>
    [[nodiscard]] static std::string optNumber(std::optional<T> v) noexcept {
        return v.has_value() ? number(static_cast<int64_t>(*v)) : std::string(null());
    }

    /**
     * @return "null" if v is empty, otherwise "true"/"false"
     */
    [[nodiscard]] static std::string optBoolean(std::optional<bool> v) noexcept {
        return v.has_value() ? std::string(boolean(*v)) : std::string(null());
    }
};

#endif //LASERGATE_V2_JSONWRITER_H
