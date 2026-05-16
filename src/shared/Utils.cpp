#include "Utils.h"

#include <sstream>
#include <algorithm>

using namespace std;

vector<string> Utils::parseString(const string& src) {
    istringstream iss(src);
    vector<string> words;
    string word;

    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}

bool Utils::prependString(string& src, const string& prefix, size_t maxLength) {
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
