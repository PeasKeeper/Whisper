#include "Server.h"
#include "IPv4Addr.h"

#include <consts.h>
#include <Utils.h>

#include <array>
#include <mutex>
#include <cstddef>
#include <vector>

#include <cstring>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

using namespace std;

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

    sockaddr_in address{};
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

        int clientFd = accept(serverFd, nullptr, nullptr);

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
    vector<thread> threadsToJoin;

    {
        lock_guard<mutex> lock(clientMutex);

        if (!clients.empty()) {
            for (auto &client : clients) {
                shutdown(client.first, SHUT_RDWR);
                close(client.first);
                if (client.second.clientThread.joinable()) {
                    threadsToJoin.push_back(std::move(client.second.clientThread));
                }
                cout << "User disconnected" << endl;
            }
            clients.clear();
        }
    }

    for (auto& clientThread : threadsToJoin) {
        clientThread.join();
    }
}

void Server::handleClient (const int clientFd) {

    array<unsigned char, FRAME_LENGTH_FIELD_SIZE> frameSizeBuf = {};

    while (true) {
        size_t receivedBytes = 0;
        size_t currentMsgSize = 0;

        while (receivedBytes < FRAME_LENGTH_FIELD_SIZE) {
            ssize_t n = recv(clientFd, frameSizeBuf.data() + receivedBytes, FRAME_LENGTH_FIELD_SIZE - receivedBytes, 0);
            if (n <= 0) {
                handleClientDisconnect(clientFd);
                return;
            }
            receivedBytes += n;
        }

        uint32_t netLength = 0;
        std::memcpy(&netLength, frameSizeBuf.data(), FRAME_LENGTH_FIELD_SIZE);

        uint32_t length = ntohl(netLength);

        string currentMessage = "";
        if (length > MAX_FRAME_SIZE) {
            handleClientDisconnect(clientFd);
            return;
        }
        currentMessage.resize(length);
        while (currentMsgSize < length) {
            ssize_t n = recv(clientFd, currentMessage.data() + currentMsgSize, length - currentMsgSize, 0);
            if (n <= 0) {
                handleClientDisconnect(clientFd);
                return;
            }
            currentMsgSize += n;
        }

        if (!running) {
            break;
        }

        unique_lock<mutex> lock(activeClientMutex);

        auto clientIt = activeClients.find(clientFd);
        if (clientIt == activeClients.cend()) {
            break;
        }

        if (clientIt->second.nickname.empty()) {
            clientIt->second.nickname = {currentMessage.data(), static_cast<size_t>(currentMsgSize)};
            continue;
        }

        string& currentUserGroup = clientIt->second.groupName;

        if (currentMessage.find("/") == 0) {
            if (currentMessage.find("/LSGRP") == 0) {
                if (groups.empty()) {
                    continue;
                }

                string answer = "";
                for (const auto &group : groups) {
                    answer += group.second.groupName;
                    if (!group.second.password.empty()) {
                        answer += u8" \U0001F512";
                    }
                    answer += "\n";
                }
                sendMessage(clientFd, answer);
                continue;
            }

            else if (currentMessage.find("/NEWGRP") == 0) {
                vector<string> words = Utils::parseString(currentMessage);
                string groupName = "";
                string groupPasswd = "";

                switch (words.size()) {
                case 3:
                    groupPasswd = words[2];
                case 2:
                    groupName = words[1];
                    break;
                default:
                    sendMessage(clientFd, "Cannot make a group with provided arguments.\n");
                    continue;
                }

                Group newGroup = {{}, groupName, groupPasswd};
                auto result = groups.emplace(groupName, newGroup);
                string answer = "";
                if (result.second) {
                    answer = "Made a new group. Name: " + groupName + ", password: " + groupPasswd + ".\n";
                }
                else {
                    answer = "Group already exists.\n";
                }
                sendMessage(clientFd, answer);
                continue;
            }

            else if (currentMessage.find("/JOINGRP") == 0) {
                if (!currentUserGroup.empty()) {
                    continue;
                }

                vector<string> words = Utils::parseString(currentMessage);
                string requestedGroupName = "";
                string sentGroupPasswd = "";

                switch (words.size()) {
                case 3:
                    sentGroupPasswd = words[2];
                case 2:
                    requestedGroupName = words[1];
                    break;
                default:
                    sendMessage(clientFd, "Cannot join a group with provided arguments.\n");
                    continue;
                }

                if (groups.find(requestedGroupName) == groups.end()) {
                    sendMessage(clientFd, "Group not found\n");
                    continue;
                }

                Group& currentGroup = groups[requestedGroupName];
                if (currentGroup.password == sentGroupPasswd) {
                    currentGroup.users.emplace(clientFd, activeClients[clientFd]);
                    currentUserGroup = requestedGroupName;
                    sendMessage(clientFd, "Joined " + requestedGroupName + "\n");
                }
                else {
                    sendMessage(clientFd, "Wrong password\n");
                }
                continue;
            }

            else if (currentMessage.find("/LEAVEGRP") == 0) {
                auto it = groups.find(currentUserGroup);
                if (it == groups.end()) {
                    continue;
                }

                it->second.users.erase(clientFd);
                string message = "Left " + currentUserGroup + "\n";

                if (it->second.users.empty()) {
                    groups.erase(currentUserGroup);
                    message += "Last user left, group destroyed\n";
                }
                sendMessage(clientFd, message);
                currentUserGroup = "";
                continue;
            }

            else {
                sendMessage(clientFd, "Incorrect command.\n");
                continue;
            }
        }

        if (currentUserGroup.empty()) {
            continue;
        }

        if (!Utils::prependString(currentMessage, activeClients[clientFd].nickname + ": ", MAX_FRAME_SIZE)) {
            sendMessage(clientFd, "Message is too long.\n");
            continue;
        }

        for (auto& client : groups[currentUserGroup].users) {
            if (client.first != clientFd) {
                sendMessage(client.first, currentMessage);
            }
        }
    }
}

ssize_t Server::sendMessage(int clientFd, const std::string& message) const {
    size_t messageSize = message.size();
    if (messageSize > MAX_FRAME_SIZE) {
        return -1;
    }

    uint32_t netLength = htonl(static_cast<uint32_t>(messageSize));

    std::vector<unsigned char> frame(FRAME_LENGTH_FIELD_SIZE + messageSize);

    std::memcpy(frame.data(), &netLength, FRAME_LENGTH_FIELD_SIZE);
    std::memcpy(frame.data() + FRAME_LENGTH_FIELD_SIZE, message.data(), messageSize);

    if (message.empty()) {
        return 0;
    }

    size_t sentBytes = 0;
    while(sentBytes < frame.size()) {
        ssize_t n = send(clientFd, reinterpret_cast<const void*>(frame.data() + sentBytes), frame.size() - sentBytes, MSG_NOSIGNAL);
        if (n <= 0) {
            return -1;
        }
        sentBytes += n;
    }
    return sentBytes;
}

void Server::handleClientDisconnect(int clientFd) {
    scoped_lock lock(activeClientMutex, clientCloseMutex);
    auto clientIt = activeClients.find(clientFd);
    if (clientIt == activeClients.end()) {
        return;
    }
    auto groupIt = groups.find(clientIt->second.groupName);
    if (groupIt != groups.end()) {
        groupIt->second.users.erase(clientIt->first);
        if (groupIt->second.users.empty()) {
            groups.erase(groupIt);
        }
    }
    clientsToClose.insert(activeClients.extract(clientIt));
}
