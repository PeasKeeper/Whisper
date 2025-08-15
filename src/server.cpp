#include "server.h"

using namespace std;

wstring getHelpMsg() {
    return L"Usage: server [options]\n\nOptions:\n  --help        Display this help message and exit\n  -p <port>     Set the server port manually (default port is used if not specified)";
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");

    int port = 3418;

    if (argc == 2) {
        if (!strcmp(argv[1], "--help")) {
            wcout << getHelpMsg() << endl;
            return 0;
        }
    }
    else if (argc == 3) {
        if (!strcmp(argv[1], "-p")) {
            port = atoi(argv[2]);
        }
        else {
            wcout << getHelpMsg() << endl;
            return 0;
        }
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket failed");
        return -1;
    }

    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

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

    wcout << L"Server up on " << getLocalIPv4() << L":" << port << endl;

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("Accept failed");
        close(server_fd);
        return -4;
    }

    wcout << L"New user connected" << endl;

    wchar_t buffer[BUFFER_SIZE] = {0};
    
    while (true) {
        if (recv(client_fd, buffer, BUFFER_SIZE*sizeof(wchar_t), 0)) { //TODO: consider windows compatibility 
            wcout << L"Client: " << buffer << endl;
            send(client_fd, static_cast<const void*>(buffer), BUFFER_SIZE*sizeof(wchar_t), 0);
        }
    }

    close(client_fd);
    close(server_fd);

    cout << L"Server shut down" << endl;
    return 0;
}