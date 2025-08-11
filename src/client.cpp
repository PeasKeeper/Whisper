#include <iostream>
#include <wchar.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

const int PORT = 12345;
const int BUFFER_SIZE = 1024;

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket failed");
        return -1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Socket failed");
        return -2;
    }

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        return -3;
    }

    wstring message = L"Hello world from client";
    send(sock, static_cast<const void*>(message.data()), message.size()*sizeof(wchar_t), 0);

    wchar_t buffer[BUFFER_SIZE] = {0};
    int bytes = recv(sock, buffer, BUFFER_SIZE*sizeof(wchar_t), 0);
    if (bytes > 0) {
        wcout << buffer << endl;
    }

    close(sock);
    return 0;
}