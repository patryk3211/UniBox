#pragma once

#include <vector>
#include <string>

namespace unibox::base64 {
    int decode(char c);
    std::vector<unsigned char> decode(const std::string& str);
}