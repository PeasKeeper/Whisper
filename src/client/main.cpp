#include "Client.h"

#include <csignal> 
#include <locale>
#include <string.h>

using namespace std;

static Client* clientInstance = nullptr; // ptr for signal handling, can not be not global

void stopSignalHandler(int signum) {
    clientInstance->stop();
}

wstring getHelpMsg() {
    return L"Usage:\n  client IP PORT\n    Connect to the server at the specified IP address and port.\n  client --help\n    Show this help message.\n";
}

int main(int argc, char *argv[]) {
    Client client;
    clientInstance = &client;

    signal(SIGINT, stopSignalHandler);

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

    int errCode = clientInstance->start(serverIP, port);

    return errCode;
}