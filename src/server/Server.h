#pragma once

#include "ClientData.h"
#include "Group.h"

#include <atomic>
#include <unordered_map>
#include <mutex>

constexpr int BUFFER_SIZE = 4096;

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
        bool prependNickname (std::string& message, const std::string& nickname);

        ssize_t sendMessage(int clientFd, const std::string& message) const;

    public:
        Server ();
        ~Server () = default;
        int start (int port);
        void stop ();
};
