#include "Server.h"

#include <iostream>

#include <clocale>
#include <csignal> 
#include <cstring>

using namespace std;

static Server* serverInstance = nullptr; // ptr for signal handling, can not be not global

void stopSignalHandler (int signum) {
    if (serverInstance != nullptr) {
        serverInstance->stop();
    }
}

string getHelpMsg () {
    return "Usage: server [options]\n\nOptions:\n  --help        Display this help message and exit\n  -p <port>     Set the server port manually (default port is used if not specified)";
}

int main (int argc, char *argv[]) {
    Server server;
    serverInstance = &server;

    setlocale(LC_ALL, "");
    signal(SIGINT, stopSignalHandler);
    
    int port = 3418;

    if (argc == 2) {
        if (!strcmp(argv[1], "--help")) {
            cout << getHelpMsg() << endl;
            return 0;
        }
        else { // for potential future flags
            cout << getHelpMsg() << endl;
            return 0;
        }
    }
    else if (argc == 3) {
        if (!strcmp(argv[1], "-p")) {
            port = atoi(argv[2]);
        }
        else {
            cout << getHelpMsg() << endl;
            return 0;
        }
    }

    int errCode = serverInstance->start(port);

    return errCode;
}
