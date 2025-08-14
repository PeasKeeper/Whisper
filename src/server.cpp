#include <iostream>
#include <wchar.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>

using namespace std;

const int PORT = 12345;
const int BUFFER_SIZE = 1024;

struct IPv4Addr { // struct is 4 bytes instead of 8 in a ptr, pass by value
    unsigned char octet[4];
};

wostream& operator<<(wostream& os, const IPv4Addr ip) {
    return os << static_cast<int>(ip.octet[0]) << "."
              << static_cast<int>(ip.octet[1]) << "."
              << static_cast<int>(ip.octet[2]) << "."
              << static_cast<int>(ip.octet[3]);
}

bool is_private_ip(const IPv4Addr ip) {
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

            if (is_private_ip(ipBytes)) {
                memcpy(result.octet, ipBytes.octet, 4);
                break;
            }
        }
    }

    freeifaddrs(ifAddrStruct);
    return result;
}


int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket failed");
        return -1;
    }

    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return -2;
    }

    if (listen(server_fd, 1) < 0) {
        perror("Listen failed");
        close(server_fd);
        return -3;
    }

    wcout << L"Server up on " << getLocalIPv4() << L":" << PORT << endl;

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("Accept failed");
        close(server_fd);
        return -4;
    }

    wcout << L"New user connected" << endl;

    wchar_t buffer[BUFFER_SIZE] = {0};
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE*sizeof(wchar_t), 0); //TODO: consider windows compatibility 
    if (bytes_received > 0) {
        wcout << L"Client: " << buffer << endl;

        wstring response = L"Hello world from server";
        send(client_fd, static_cast<const void*>(response.data()), response.size()*sizeof(wchar_t), 0);
    }

    close(client_fd);
    close(server_fd);

    cout << L"Server shut down" << endl;
    return 0;
}