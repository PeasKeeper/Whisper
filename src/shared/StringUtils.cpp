#include "StringUtils.h"

#include <sstream>

using namespace std;

vector<string> StringUtils::parseString(const string& src) {
    istringstream iss(src);
    vector<string> words;
    string word;

    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}

bool StringUtils::prependString(string& src, const string& prefix, size_t maxLength) {
    if (prefix.size() + src.size() > maxLength) {
        return false;
    }

    src = prefix + src;
    return true;
}

vector<byte> StringUtils::stringToBytes(const string& src) {
    const auto* begin = reinterpret_cast<const byte*>(src.data());
    return {begin, begin + src.size()};
}

string StringUtils::bytesToString(const vector<byte>& src) {
    const auto* begin = reinterpret_cast<const char*>(src.data());
    return {begin, begin + src.size()};
}
