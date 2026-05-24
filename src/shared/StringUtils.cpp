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

vector<string> StringUtils::splitString(const string& src, char separator) {
    istringstream stream(src);
    vector<string> parts;
    string part;

    while (getline(stream, part, separator)) {
        parts.push_back(part);
    }

    return parts;
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

bool StringUtils::isSpace(char character) {
    return character == ' ' ||
           character == '\n' ||
           character == '\t' ||
           character == '\r';
}

string StringUtils::wrapText(
    const string& text,
    int maxWidth,
    WidthFunction widthFn,
    SplitFunction splitFn
) {
    if (maxWidth <= 0) {
        return text;
    }
    string output;
    int lineWidth = 0;

    auto appendLongWord = [&](const string& word) {
        if (lineWidth > 0) {
            output += '\n';
            lineWidth = 0;
        }
        for (const string& glyph : splitFn(word)) {
            const int glyphWidth = widthFn(glyph);
            if (lineWidth > 0 && lineWidth + glyphWidth > maxWidth) {
                output += '\n';
                lineWidth = 0;
            }
            output += glyph;
            lineWidth += glyphWidth;
        }
    };

    auto appendWord = [&](const string& word) {
        const int wordWidth = widthFn(word);
        if (wordWidth > maxWidth) {
            appendLongWord(word);
            return;
        }
        if (lineWidth == 0) {
            output += word;
            lineWidth = wordWidth;
            return;
        }
        if (lineWidth + 1 + wordWidth <= maxWidth) {
            output += ' ';
            output += word;
            lineWidth += 1 + wordWidth;
            return;
        }
        output += '\n';
        output += word;
        lineWidth = wordWidth;
    };

    size_t index = 0;
    while (index < text.size()) {
        if (text[index] == '\n') {
            output += '\n';
            lineWidth = 0;
            ++index;
            continue;
        }
        while (index < text.size() && isSpace(text[index]) && text[index] != '\n') {
            ++index;
        }
        const size_t wordStart = index;
        while (index < text.size() && !isSpace(text[index])) {
            ++index;
        }
        if (wordStart == index) {
            continue;
        }
        appendWord(text.substr(wordStart, index - wordStart));
    }
    return output;
}
