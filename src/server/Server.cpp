#include "Server.h"
#include "IPv4Addr.h"

#include <array>
#include <vector>
#include <sstream>

#include <cstring>

#include <unistd.h>
#include <arpa/inet.h>

using namespace std;

namespace {

vector<string> parseString (string src) {
    istringstream iss(src);
    std::vector<std::string> words;
    std::string word;

    while (iss >> word) {
        words.push_back(word);
    }
    return words;
}

}

Server::Server () {
    running = true;
    serverFd = -1;
}

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

    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(serverFd);
        return -4;
    }

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

    cout << "Server up on " << getLocalIPv4() << ":" << port << endl;

    while (running) {
        closeClients(clientsToClose, clientCloseMutex);

        int clientFd = accept(serverFd, NULL, NULL);

        if (!running) {
            break;
        }

        if (clientFd < 0) {
            perror("Accept failed");
            continue;
        }

        unique_lock<mutex> lock(activeClientMutex);

        auto emplaceData = activeClients.emplace(clientFd, ClientData{clientFd});

        lock.unlock();

        cout << "New user connected" << endl;
        
        thread t(&Server::handleClient, this, clientFd);
        emplaceData.first->second.clientThread = std::move(t);

    }
    closeClients(clientsToClose, clientCloseMutex);
    closeClients(activeClients, activeClientMutex);

    return 0;
}

void Server::stop () {
    running = false;
    if (serverFd >= 0) {
        close(serverFd);
    }
    cout << "\nServer shut down" << endl;
}

void Server::closeClients (unordered_map<int, ClientData>& clients, mutex& clientMutex) {
    lock_guard<mutex> lock(clientMutex);

    if (!clients.empty()) {
        for (auto &client : clients) {
            shutdown(client.first, SHUT_RDWR);
            close(client.first);
            if (client.second.clientThread.joinable()) {
                client.second.clientThread.join();
            }
            cout << "User disconnected" << endl;
        }
        clients.clear();
    }
}

void Server::handleClient (const int clientFd) {

    array<char, BUFFER_SIZE> buffer = {};
    int currentMsgSize = 0;

    while (true) {
        currentMsgSize = recv(clientFd, buffer.data(), buffer.size()-1, 0);

        if (!running) {
            break;
        }

        if (currentMsgSize > 0) {
            lock_guard<mutex> lock(activeClientMutex);

            if (activeClients[clientFd].nickname == "") {
                activeClients[clientFd].nickname = {buffer.data(), static_cast<size_t>(currentMsgSize-1)};
                continue;
            }

            currentMsgSize--;
            string currentMessage = string(buffer.data(), currentMsgSize);

            if (currentMessage.find("/NEWGRP") == 0) {
                vector<string> words = parseString(currentMessage);
                string& groupName = words[1]; // just for clarity
                string& groupPasswd = words[2];
                Group newGroup = {{}, groupName, groupPasswd};
                groups.emplace(groupName, newGroup);
                continue;
            }

            if (currentMessage.find("/JOINGRP") == 0) {
                vector<string> words = parseString(currentMessage);
                string& requestedGroupName = words[1]; // just for clarity
                string& sentGroupPasswd = words[2];
                if (groups.find(requestedGroupName) != groups.end()) {
                    if (groups[requestedGroupName].password == sentGroupPasswd) {
                        groups[requestedGroupName].users.emplace(clientFd, activeClients[clientFd]);
                        activeClients[clientFd].groupName = requestedGroupName;
                    }
                    else {
                        // wrong password
                        // TODO: add handling
                    }
                }
                else {
                    // wrong name
                    // TODO: add handling
                }
                continue;
            }

            if (currentMessage.find("/LEAVEGRP") == 0) {
                string& currentUserGroup = activeClients[clientFd].groupName;
                if (groups[currentUserGroup].users.size() == 1) {
                    groups.erase(activeClients[clientFd].groupName);
                }

                groups[currentUserGroup].users.erase(clientFd);
                currentUserGroup = "";
                continue;
            }

            if (!prependNickname(currentMessage, activeClients[clientFd].nickname, currentMsgSize)) {
                // ignore for now
                // TODO: add handling
                continue;
            }

            const string& currentUserGroup = activeClients[clientFd].groupName;
            for (auto& client : groups[currentUserGroup].users) {
                if (client.first != clientFd) {
                    send(client.first, static_cast<const void*>(currentMessage.data()), currentMsgSize+1, 0);
                }
            }
        }
        else if (!currentMsgSize){
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

bool Server::prependNickname(string& message, const string& nickname, int& msgSize) {
    if (msgSize + nickname.size() >= BUFFER_SIZE) {
        return false;
    }

    string tempNick = nickname + ": ";

    string res = tempNick + string(message.data(), msgSize);
    msgSize = res.size();
    message.resize(msgSize);
    copy(res.begin(), res.end(), message.begin());
    message[res.size()] = '\0';

    return true;
}
