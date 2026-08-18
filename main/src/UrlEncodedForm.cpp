#include "UrlEncodedForm.h"

namespace {

// -1 if c isn't a hex digit, no exceptions on embedded
int hexDigitToInt(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string decodeUrlComponent(std::string_view value) {
    std::string result;
    result.reserve(value.size());

    for (size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];

        if (c == '+') {
            result += ' ';
            continue;
        }

        if (c == '%' && i + 2 < value.size()) {
            const int hi = hexDigitToInt(value[i + 1]);
            const int lo = hexDigitToInt(value[i + 2]);

            if (hi >= 0 && lo >= 0) {
                result += static_cast<char>((hi << 4) | lo);
                i += 2;
                continue;
            }
        }

        result += c;
    }

    return result;
}

}

std::unordered_map<std::string, std::string> UrlEncodedForm::parse(std::string_view body) noexcept {
    std::unordered_map<std::string, std::string> values;

    size_t pos = 0;
    while (pos < body.size()) {
        const size_t ampPos = body.find('&', pos);
        const std::string_view pair = (ampPos == std::string_view::npos)
            ? body.substr(pos)
            : body.substr(pos, ampPos - pos);

        const size_t eqPos = pair.find('=');
        if (eqPos != std::string_view::npos) {
            const std::string_view key = pair.substr(0, eqPos);
            const std::string_view value = pair.substr(eqPos + 1);
            values[decodeUrlComponent(key)] = decodeUrlComponent(value);
        }

        if (ampPos == std::string_view::npos) {
            break;
        }
        pos = ampPos + 1;
    }

    return values;
}
