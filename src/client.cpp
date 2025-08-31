#include "client.h"
#include <wchar.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <locale>
#include <codecvt>
#include <bits/stdc++.h>
#include <mutex>
#include <csignal> 
#include <atomic>

using namespace std;

static atomic<bool> running(true);

static int sock = -1;

void stopSignalHandler(int signum) {
    
    running = false; 
    close(sock);
    terminate();

}

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

    sock = socket(AF_INET, SOCK_STREAM, 0);
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

    thread inputThread = thread([&]{
        while(running) {
            getline(wcin, message, L'\n');
            if (message.size() > 0) {
                send(sock, reinterpret_cast<const void*>(message.data()), (message.size()+1) * sizeof(wchar_t), 0);
            }
        }
    });

    while (running) {
        int bytes = recv(sock, buffer, (BUFFER_SIZE - 1)*sizeof(wchar_t), 0);

        if (bytes > 0) {
            wcout << buffer << endl;
        }

        else if (!bytes){
            wcout << L"\nServer shut down" << endl;
            close(sock);
            terminate();
        }

        else {
            continue; // TODO: add error handling
        }
    }

    inputThread.join();

    return 0;
}