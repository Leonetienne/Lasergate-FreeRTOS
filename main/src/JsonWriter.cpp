#include "JsonWriter.h"
#include <cstdio>

std::string JsonWriter::escape(std::string_view s) noexcept {
    std::string out;
    out.reserve(s.size());

    for (const char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[7];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }

    return out;
}

std::string JsonWriter::string(std::string_view s) noexcept {
    return "\"" + escape(s) + "\"";
}

const char* JsonWriter::boolean(bool v) noexcept {
    return v ? "true" : "false";
}

std::string JsonWriter::number(int64_t v) noexcept {
    return std::to_string(v);
}

const char* JsonWriter::null() noexcept {
    return "null";
}
