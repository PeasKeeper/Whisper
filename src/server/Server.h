#pragma once

#include "ClientData.h"
#include "Group.h"

#include <atomic>
#include <unordered_map>
#include <mutex>

#include <sys/types.h>

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

        ssize_t sendMessage (int clientFd, const std::string& message) const;
        void handleClientDisconnect (int clientFd);

    public:
        Server ();
        ~Server () = default;
        int start (int port);
        void stop ();
};
