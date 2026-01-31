#pragma once

#include "ClientData.h"

#include <iostream>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <array>

constexpr int BUFFER_SIZE = 4096;

class Server {
    private:
        int serverFd;

        std::atomic<bool> running;

        std::mutex activeClientMutex;
        std::mutex clientCloseMutex;

        std::unordered_map<int, ClientData> activeClients;
        std::unordered_map<int, ClientData> clientsToClose;

        void handleClient (const int clientFd);
        void closeClients (std::unordered_map<int, ClientData>& clients, std::mutex& clientMutex);
        bool prependNickname (std::array<char, BUFFER_SIZE>& buffer, const std::string& nickname, int& msgSize);
        // not making buffer a class field bc
        // it shouldn't be accesible from everywhere

    public:
        Server ();
        ~Server () {};
        int start (int port);
        void stop ();
};
