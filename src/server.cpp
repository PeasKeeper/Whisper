#include "server.h"
#include "IPv4Addr.h"
#include "ClientData.h"
#include <wchar.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <locale>
#include <codecvt>
#include <bits/stdc++.h>
#include <mutex>
#include <csignal> 
#include <atomic>

using namespace std; // TODO: turn the whole thing into a class, too many globals

static mutex clientMutex;
static mutex clientDeleteMutex;

static atomic<bool> running(true);

static int serverFd = -1;

static unordered_map<int, ClientData> clients;
static unordered_map<int, ClientData> clientsToClose;

void stopSignalHandler(int signum) {
    
    running = false; 
    close(serverFd);

}

wstring getHelpMsg() {
    return L"Usage: server [options]\n\nOptions:\n  --help        Display this help message and exit\n  -p <port>     Set the server port manually (default port is used if not specified)";
}

void handleClient (const int clientFd) {

    wchar_t buffer[BUFFER_SIZE] = {0};
    int bytes = 0;

    while (true) {
        bytes = recv(clientFd, buffer, (BUFFER_SIZE - 1)*sizeof(wchar_t), 0);
        if (!running) {
            break;
        }
        if (bytes > 0) { //TODO: consider windows compatibility 

            lock_guard<mutex> lock(clientMutex);

            for (auto &client : clients) {
                if (client.first != clientFd) {
                    send(client.first, static_cast<const void*>(buffer), bytes + sizeof(wchar_t), 0);
                }
            }
        }

        else if (!bytes){
            scoped_lock lock(clientMutex, clientDeleteMutex);
            clientsToClose.insert(clients.extract(clientFd));
            break;
        }

        else {
            continue; // TODO: add error handling
        }
    }
    return;
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");
    signal(SIGINT, stopSignalHandler);
    
    int port = 3418;

    if (argc == 2) {
        if (!strcmp(argv[1], "--help")) {
            wcout << getHelpMsg() << endl;
            return 0;
        }
        else { // for potential future flags
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

    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd == -1) {
        perror("Socket failed");
        return -1;
    }

    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(serverFd);
        return -2;
    }

    if (listen(serverFd, 1) < 0) {
        perror("Listen failed");
        close(serverFd);
        return -3;
    }

    wcout << L"Server up on " << getLocalIPv4() << L":" << port << endl;

    while (running) {

        int clientFd = accept(serverFd, NULL, NULL);

        if (clientFd < 0) {
            perror("Accept failed");
            continue;
        }

        if (!running) {
            break;
        }

        unique_lock<mutex> lock(clientMutex);

        auto emplaceData = clients.emplace(clientFd, ClientData());

        lock.unlock();

        wcout << L"New user connected" << endl;
        
        thread t(handleClient, clientFd);
        emplaceData.first->second.clientThread = (move(t));

        unique_lock<mutex> deleteLock(clientDeleteMutex);

        if (!clientsToClose.empty()) {

            for (auto &client : clientsToClose) {
                shutdown(client.first, SHUT_RDWR);
                close(client.first);
                client.second.clientThread.join();
                wcout << L"User disconnected" << endl;
            }

            clientsToClose.clear();
        }

        deleteLock.unlock();
    }

    unique_lock<mutex> lock(clientMutex);

    for (auto &client : clients) {
        shutdown(client.first, SHUT_RDWR);
        close(client.first);
        if (client.second.clientThread.joinable()) {
            client.second.clientThread.join();
        }
        wcout << L"User disconnected" << endl;
    }

    lock.unlock();

    unique_lock<mutex> deleteLock(clientDeleteMutex);

    for (auto &client : clientsToClose) {
        shutdown(client.first, SHUT_RDWR);
        close(client.first);
        if (client.second.clientThread.joinable()) {
            client.second.clientThread.join();
        }
        wcout << L"User disconnected" << endl;
    }

    deleteLock.unlock();

    wcout << L"\nServer shut down" << endl;
    return 0;
}