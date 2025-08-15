#include "IPv4Addr.h"
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>

using namespace std;

wostream& operator<<(wostream& os, const IPv4Addr ip) {
    return os << static_cast<int>(ip.octet[0]) << "."
              << static_cast<int>(ip.octet[1]) << "."
              << static_cast<int>(ip.octet[2]) << "."
              << static_cast<int>(ip.octet[3]);
}

bool isPrivateIp(const IPv4Addr ip) {
    return (ip.octet[0] == 192 && ip.octet[1] == 168) ||
           (ip.octet[0] == 10) ||
           (ip.octet[0] == 172 && ip.octet[1] >= 16 && ip.octet[1] <= 31);
}

IPv4Addr getLocalIPv4() {
    ifaddrs *ifAddrStruct = nullptr;
    ifaddrs *ifa = nullptr;

    IPv4Addr result{{0, 0, 0, 0}};

    if (getifaddrs(&ifAddrStruct) == -1) {
        perror("Failed to find local address");
        return result;
    }

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }

        if (ifa->ifa_addr->sa_family == AF_INET) {
            sockaddr_in *sa = (sockaddr_in *)ifa->ifa_addr;

            IPv4Addr ipBytes;
            memcpy(ipBytes.octet, &sa->sin_addr, 4);

            if (ipBytes.octet[0] == 127) {
                continue; // loopback
            }

            if (isPrivateIp(ipBytes)) {
                memcpy(result.octet, ipBytes.octet, 4);
                break;
            }
        }
    }

    freeifaddrs(ifAddrStruct);
    return result;
}