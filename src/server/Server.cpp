#include "Server.h"
#include "IPv4Addr.h"
#include "StopReason.h"

#include <consts.h>
#include <StringUtils.h>
#include <SocketAdapter.h>

#include <mutex>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>

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
    std::vector<int> brokenClients;

    while (true) {
        ReceiveResult result = SocketAdapter::receiveMessage(clientFd);
        if (result.stopReason != StopReason::None) {
            handleClientDisconnect(clientFd);
            break;
        }
        string currentMessage = StringUtils::bytesToString(result.payload);

        if (!running) {
            break;
        }

        unique_lock<mutex> lock(activeClientMutex);

        auto clientIt = activeClients.find(clientFd);
        if (clientIt == activeClients.cend()) {
            break;
        }

        if (clientIt->second.nickname.empty()) {
            clientIt->second.nickname = currentMessage;
            continue;
        }

        string currentUserGroup = clientIt->second.groupName;

        if (currentMessage.find("/") == 0) {
            string answer = parseCommand(currentMessage, clientFd);
            if (answer.empty()) break;
            if (!sendOrMarkBroken(clientFd, answer, brokenClients)) break;
            continue;
        }

        auto groupIt = groups.find(currentUserGroup);
        if (groupIt == groups.end()) {
            continue;
        }

        if (!StringUtils::prependString(currentMessage, clientIt->second.nickname + ": ", MAX_FRAME_SIZE)) {
            if (!sendOrMarkBroken(clientFd, "Message is too long.\n", brokenClients)) break;
            continue;
        }

        for (auto& client : groupIt->second.users) {
            if (client.first != clientFd) {
                if(!sendOrMarkBroken(client.first, currentMessage, brokenClients)) {
                    continue;
                }
            }
        }

        lock.unlock();
        for (auto& client : brokenClients) {
            handleClientDisconnect(client);
        }
        brokenClients.clear();
    }
    for (auto& client : brokenClients) {
        handleClientDisconnect(client);
    }
    brokenClients.clear();
}

bool Server::sendOrMarkBroken(int clientFd, const std::string& message, vector<int>& brokenClients) {
    StopReason result = SocketAdapter::sendMessage(clientFd, StringUtils::stringToBytes(message));
    if (result != StopReason::None) {
        brokenClients.push_back(clientFd);
        return false;
    }
    return true;
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

 std::string Server::parseCommand(const string& message, int clientFd) {
     string answer = "";
     auto clientIt = activeClients.find(clientFd);

     if (clientIt == activeClients.end()) {
         return "";
     }

     string currentUserGroup = clientIt->second.groupName;

     if (message.find("/LSGRP") == 0) {
         if (groups.empty()) {
             return "Currently there are no active groups.\n";
         }

         for (const auto &group : groups) {
             answer += group.second.groupName;
             if (!group.second.password.empty()) {
                 answer += u8" \U0001F512";
             }
             answer += "\n";
         }
     }

     else if (message.find("/NEWGRP") == 0) {
         vector<string> words = StringUtils::parseString(message);
         string groupName = "";
         string groupPasswd = "";

         switch (words.size()) {
         case 3:
             groupPasswd = words[2];
         case 2:
             groupName = words[1];
             break;
         default:
             return "Cannot make a group with provided arguments.\n";
         }

         Group newGroup = {{}, groupName, groupPasswd};
         auto result = groups.emplace(groupName, newGroup);
         if (result.second) {
             answer = "Made a new group. Name: " + groupName + ", password: " + groupPasswd + ".\n";
         }
         else {
             answer = "Group already exists.\n";
         }
     }

     else if (message.find("/JOINGRP") == 0) {
         if (!currentUserGroup.empty()) {
             return "You are already in a group. Leave it first, to join another group.\n";
         }

         vector<string> words = StringUtils::parseString(message);
         string requestedGroupName = "";
         string sentGroupPasswd = "";

         switch (words.size()) {
         case 3:
             sentGroupPasswd = words[2];
         case 2:
             requestedGroupName = words[1];
             break;
         default:
             return "Cannot join a group with provided arguments.\n";
         }

         auto requestedGroupIt = groups.find(requestedGroupName);

         if (requestedGroupIt == groups.end()) {
             return "Group not found\n";
         }

         if (requestedGroupIt->second.password == sentGroupPasswd) {
             requestedGroupIt->second.users.emplace(clientFd, activeClients[clientFd]);
             clientIt->second.groupName = requestedGroupName;
             answer = "Joined " + requestedGroupName + "\n";
         }
         else {
             answer = "Wrong password\n";
         }
     }

     else if (message.find("/LEAVEGRP") == 0) {
         auto it = groups.find(currentUserGroup);
         if (it == groups.end()) {
             return "You are not currently in a group.\n";
         }

         it->second.users.erase(clientFd);
         answer = "Left " + currentUserGroup + "\n";

         if (it->second.users.empty()) {
             groups.erase(currentUserGroup);
             answer += "Last user left, group destroyed\n";
         }
         clientIt->second.groupName.clear();
     }

     else {
         answer = "Incorrect command.\n";
     }
     return answer;
 }
