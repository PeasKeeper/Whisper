#pragma once

#include "ClientData.h"

#include <iostream>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <array>

constexpr int BUFFER_SIZE = 1024;

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
        void prependNickname (std::array<wchar_t, BUFFER_SIZE>& buffer, const std::wstring& nickname); // not making buffer a class field bc 
                                                                                                       // it shouldn't be accesible from everywhere

    public:
        Server ();
        ~Server () {};
        int start (int port);
        void stop ();
};