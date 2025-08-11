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

    wcout << L"Server up on port " << PORT << endl;

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("Accept failed");
        close(server_fd);
        return -4;
    }

    wcout << L"New user connected" << endl;

    wchar_t buffer[BUFFER_SIZE] = {0};
    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE*sizeof(wchar_t), 0);
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