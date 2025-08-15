#include "client.h"

using namespace std;

wstring getHelpMsg() {
    return L"Usage:\n  client IP PORT\n    Connect to the server at the specified IP address and port.\n  client --help\n    Show this help message.\n";
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");

    int port = 0;
    char* serverIP;

    if (argc == 2) {
        if (!strcmp(argv[1], "--help")) {
            wcout << getHelpMsg() << endl;
            return 0;
        }
    }
    if (argc == 3) {
        serverIP = argv[1];
        port = atoi(argv[2]);
    }
    else {
        wcout << getHelpMsg() << endl;
        return 0;
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket failed");
        return -1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, serverIP, &server_addr.sin_addr) <= 0) {
        perror("Socket failed");
        return -2;
    }

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        return -3;
    }

    wstring message = L"";
    wchar_t buffer[BUFFER_SIZE] = {0};

    while(true) {
        getline(wcin, message);

        send(sock, static_cast<const void*>(message.data()), message.size()*(sizeof(wchar_t)+1), 0);

        if (recv(sock, buffer, BUFFER_SIZE*sizeof(wchar_t), 0)) {
            wcout << buffer << endl;
        }
    }

    close(sock);
    return 0;
}