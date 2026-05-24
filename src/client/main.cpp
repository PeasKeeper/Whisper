#include "Client.h"
#include "UiManager.h"

#include <StopReason.h>

#include <iostream>

#include <csignal>
#include <clocale>
#include <cstring>

using namespace std;

static Client* clientInstance = nullptr; // ptr for signal handling, can not be not global
static ftxui::UiManager* uiInstance = nullptr; // ptr for signal handling, can not be not global

void stopSignalHandler(int signum) {
    if (uiInstance != nullptr) {
        uiInstance->stop();
    }
    if (clientInstance != nullptr) {
        clientInstance->stop(StopReason::LocalUser);
        clientInstance->joinThreads();
    }
}

string getHelpMsg() {
    return "Usage:\n  client IP PORT Nickname\n    Connect to the server at the specified IP address and port using Nickname.\n  client --help\n    Show this help message.\n";
}

int main(int argc, char *argv[]) {
    Client client;
    clientInstance = &client;
    ftxui::UiManager manager(*clientInstance);
    uiInstance = &manager;

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

    clientInstance->setUserMessageCallback(
        [&](std::string author, std::string body) {
            manager.postUserMessage(std::move(author), std::move(body));
        }
    );

    clientInstance->setSystemMessageCallback(
        [&](std::string body) {
            manager.postSystemMessage(std::move(body));
        }
    );

    clientInstance->setStopCallback(
        [&]() {
            manager.stop();
        }
    );

    int errCode = clientInstance->start(serverIP, port, nickname);
    if (errCode) {
        return errCode;
    }

    manager.run();

    client.stop(StopReason::LocalUser);
    manager.stop();
    client.joinThreads();

    return errCode;
}
