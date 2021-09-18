#include <util/base64.hpp>

using namespace unibox;

int unibox::base64::decode(char c) {
    if(c >= 'A' && c <= 'Z') return c - 'A';
    else if(c >= 'a' && c <= 'z') return c - 'a' + 0b011010;
    else if(c >= '0' && c <= '9') return c - '0' + 0b110100;
    else if(c == '+') return 0b111110;
    else if(c == '/') return 0b111111;
    else if(c == '=') return 0b1000000; // Padding
    else return -1; // Invalid character
}

std::vector<unsigned char> unibox::base64::decode(const std::string& str) {
    std::vector<unsigned char> data;
    unsigned char packet = 0;
    int left = 8;
    for(int i = 0; i < str.length(); i++) {
        int value = decode(str[i]);
        int cur_left = 6;
        while(cur_left > 0) {
            if(value >= 0 && value < 64) {
                int shamt = left-cur_left;
                if(shamt > 0) {
                    packet |= (value << shamt);
                    left = shamt;
                    break; // If the shift amount is greater than 0 then we have utilized the entire 6 bit value so we can continue to the next one.
                } else if(shamt == 0) {
                    packet |= value;
                    // We have a full byte, push it into the return data.
                    data.push_back(packet);
                    packet = 0;
                    left = 8;
                    break; // Shift amount is equal to 0, we utilized the 6 bits.
                } else {
                    packet |= (value >> -shamt);
                    cur_left -= left;
                    // We have a full byte.
                    data.push_back(packet);
                    packet = 0;
                    left = 8;
                }
            } else if(value == 64) {
                if(left < 8 && packet != 0) {
                    data.push_back(packet);
                    left = 8;
                    packet = 0;
                }
                break;
            } else cur_left = 0;
        }
    }
    return data;
}