#include "AppManager.h"

#include <StopReason.h>

#include <iostream>

#include <csignal>
#include <clocale>
#include <cstring>

using namespace std;

namespace {

static AppManager* appInstance = nullptr;

void stopSignalHandler(int signum) {
    if (appInstance != nullptr) {
        appInstance->requestStop(StopReason::LocalUser);
    }
}

string getHelpMsg() {
    return "Usage:\n  client IP PORT Nickname\n    Connect to the server at the specified IP address and port using Nickname.\n  client --help\n    Show this help message.\n";
}

} // namespace

int main(int argc, char *argv[]) {
    signal(SIGINT, stopSignalHandler);

    setlocale(LC_ALL, "");

    int port = 0;
    char* serverIP = nullptr;
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
        if ((!strcmp(argv[3], "")) || (strchr(argv[3], '\x1F') != nullptr)) {
            cout << "Invalid nickname." << endl;
            return -1;
        }
        nickname = argv[3];
    }
    else {
        cout << getHelpMsg() << endl;
        return 0;
    }

    AppManager appManager(serverIP, port, nickname);
    appInstance = &appManager;
    appManager.run();
    appManager.finalize();
    appManager.printStopMessage();

    return 0;
}
