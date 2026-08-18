#ifndef LASERGATE_V2_URLENCODEDFORM_H
#define LASERGATE_V2_URLENCODEDFORM_H

#include <string>
#include <string_view>
#include <unordered_map>

/**
 * Parses an "a=1&b=2" application/x-www-form-urlencoded request body into a key/value map
 */
class UrlEncodedForm {
public:
    /**
     * @param body request body
     * @return the decoded key/value pairs found in body
     */
    [[nodiscard]] static std::unordered_map<std::string, std::string> parse(std::string_view body) noexcept;
};

#endif //LASERGATE_V2_URLENCODEDFORM_H
