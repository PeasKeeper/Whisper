#include "Server.h"
#include "IPv4Addr.h"

#include <wchar.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

using namespace std;

Server::Server () {
    running = true;
    serverFd = -1;
}

Server::~Server () {}

int Server::start (int port) {
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

        if (clientFd < 0 && running) {
            perror("Accept failed");
            continue;
        }

        if (!running) {
            break;
        }

        unique_lock<mutex> lock(activeClientMutex);

        auto emplaceData = activeClients.emplace(clientFd, ClientData());

        lock.unlock();

        wcout << L"New user connected" << endl;
        
        thread t(&Server::handleClient, this, clientFd);
        emplaceData.first->second.clientThread = (move(t));

        closeClients(clientsToClose, clientCloseMutex);
    }
    closeClients(activeClients, activeClientMutex);
    closeClients(clientsToClose, clientCloseMutex);

    return 0;
}

void Server::stop () {
    running = false;
    close(serverFd);
    wcout << L"\nServer shut down" << endl;
}

void Server::closeClients (unordered_map<int, ClientData>& clients, mutex& clientMutex) {
    unique_lock<mutex> lock(clientMutex);

    if (!clients.empty()) {
        for (auto &client : clients) {
            shutdown(client.first, SHUT_RDWR);
            close(client.first);
            if (client.second.clientThread.joinable()) {
                client.second.clientThread.join();
            }
            wcout << L"User disconnected" << endl;
        }
        clients.clear();
    }
}

void Server::handleClient (const int clientFd) {

    wchar_t buffer[BUFFER_SIZE] = {0};
    int bytes = 0;

    while (true) {
        bytes = recv(clientFd, buffer, (BUFFER_SIZE - 1)*sizeof(wchar_t), 0);

        if (!running) {
            break;
        }

        if (bytes > 0) { //TODO: consider windows compatibility 

            lock_guard<mutex> lock(activeClientMutex);

            for (auto &client : activeClients) {
                if (client.first != clientFd) {
                    send(client.first, static_cast<const void*>(buffer), bytes + sizeof(wchar_t), 0);
                }
            }
        }

        else if (!bytes){
            scoped_lock lock(activeClientMutex, clientCloseMutex);
            clientsToClose.insert(activeClients.extract(clientFd));
            break;
        }

        else {
            continue; // TODO: add error handling
        }
    }
    return;
}