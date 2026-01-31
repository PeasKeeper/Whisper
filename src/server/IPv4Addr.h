#pragma once

#include <iostream>

struct IPv4Addr { // struct is 4 bytes instead of 8 in a ptr, pass by value
    unsigned char octet[4];
};

std::ostream& operator<<(std::ostream& os, const IPv4Addr ip);

bool isPrivateIp(const IPv4Addr ip);

IPv4Addr getLocalIPv4();
