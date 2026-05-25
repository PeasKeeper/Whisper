#pragma once

#include "ClientData.h"
#include "Group.h"

#include <atomic>
#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

class Server {
    private:
        int serverFd;

        std::atomic<bool> running;

        std::mutex activeClientMutex;
        std::mutex clientCloseMutex;

        std::unordered_map<int, ClientData> activeClients;
        std::unordered_map<int, ClientData> clientsToClose;

        std::unordered_map<std::string, Group> groups;

        void handleClient (const int clientFd);
        void closeClients (std::unordered_map<int, ClientData>& clients, std::mutex& clientMutex);

        bool sendOrMarkBroken (int clientFd, const std::string& message, std::vector<int>& brokenClients);
        void handleClientDisconnect (int clientFd);

        std::string parseCommand(const std::string& message, int clientFd);

    public:
        Server ();
        ~Server () = default;
        int start (int port);
        void stop ();
};
