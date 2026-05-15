#pragma once

#include <vector>
#include <string>

class Utils {
    public:
        Utils() = default;
        ~Utils() = default;
        static std::vector<std::string> parseString(const std::string& src);
        static bool prependString(std::string& src, const std::string& prefix, size_t maxLength);
};
