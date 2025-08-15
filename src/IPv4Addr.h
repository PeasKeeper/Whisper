#ifndef IPV4ADDR_H

#include <iostream>

struct IPv4Addr { // struct is 4 bytes instead of 8 in a ptr, pass by value
    unsigned char octet[4];
};

std::wostream& operator<<(std::wostream& os, const IPv4Addr ip);

bool isPrivateIp(const IPv4Addr ip);

IPv4Addr getLocalIPv4();

#endif