#include "StringUtils.h"

#include <sstream>
#include <algorithm>

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
    size_t srcSize = src.size();
    const size_t resultSize = prefix.size() + srcSize;

    if (resultSize > maxLength) {
        return false;
    }

    string res = prefix + string(src.data(), srcSize);
    srcSize = res.size();
    src.resize(srcSize);
    copy(res.begin(), res.end(), src.begin());

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
