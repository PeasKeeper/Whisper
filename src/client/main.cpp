#include "Client.h"

#include <csignal>
#include <locale.h>
#include <string.h>
#include <iostream>

using namespace std;

static Client* clientInstance = nullptr; // ptr for signal handling, can not be not global

void stopSignalHandler(int signum) {
    if (clientInstance != nullptr) {
        clientInstance->stop();
    }
}

string getHelpMsg() {
    return "Usage:\n  client IP PORT\n    Connect to the server at the specified IP address and port.\n  client --help\n    Show this help message.\n";
}

int main(int argc, char *argv[]) {
    Client client;
    clientInstance = &client;

    signal(SIGINT, stopSignalHandler);

    setlocale(LC_ALL, "");

    int port = 0;
    char* serverIP;
    string nickname = "";

    if (argc == 2) {
        if (!strcmp(argv[1], "--help")) {
            cout << getHelpMsg() << endl;
            return 0;
        }
    }
    if (argc == 4) {
        serverIP = argv[1];
        port = atoi(argv[2]);
        nickname = argv[3];
    }
    else {
        cout << getHelpMsg() << endl;
        return 0;
    }

    int errCode = clientInstance->start(serverIP, port, nickname);

    return errCode;
}
