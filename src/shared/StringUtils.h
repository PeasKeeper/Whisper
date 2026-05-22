#pragma once

#include <vector>
#include <string>
#include <cstddef>

class StringUtils {
    public:
        StringUtils() = default;
        ~StringUtils() = default;
        static std::vector<std::string> parseString(const std::string& src);
        static std::vector<std::string> splitString(const std::string& src, char separator);
        static bool prependString(std::string& src, const std::string& prefix, std::size_t maxLength);
        static std::vector<std::byte> stringToBytes(const std::string& src);
        static std::string bytesToString(const std::vector<std::byte>& src);
};
