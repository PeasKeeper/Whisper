#include "Client.h"

#include <csignal> 
#include <locale>
#include <string.h>
#include <codecvt>

using namespace std;

static Client* clientInstance = nullptr; // ptr for signal handling, can not be not global

void stopSignalHandler(int signum) {
    if (clientInstance != nullptr) {
        clientInstance->stop();
    }
}

wstring getHelpMsg() {
    return L"Usage:\n  client IP PORT\n    Connect to the server at the specified IP address and port.\n  client --help\n    Show this help message.\n";
}

/*wstring toWstring(const char& src) {
    static wstring_convert<codecvt_utf8_utf16<wchar_t>> conv;
    return conv.from_bytes(src);
}  make smth like this later */

int main(int argc, char *argv[]) {
    Client client;
    clientInstance = &client;

    signal(SIGINT, stopSignalHandler);

    setlocale(LC_ALL, "");

    int port = 0;
    char* serverIP;
    wstring nickname = L"";

    if (argc == 2) {
        if (!strcmp(argv[1], "--help")) {
            wcout << getHelpMsg() << endl;
            return 0;
        }
    }
    if (argc == 4) {
        serverIP = argv[1];
        port = atoi(argv[2]);
        nickname = wstring(argv[3], argv[3] + strlen(argv[3])); //temporary
    }
    else {
        wcout << getHelpMsg() << endl;
        return 0;
    }

    int errCode = clientInstance->start(serverIP, port, nickname);

    return errCode;
}