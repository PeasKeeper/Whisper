#pragma once

#include <iostream>
#include <atomic>

constexpr int BUFFER_SIZE = 1024;

class Client {
    private:
        int sock;

        std::atomic<bool> running;

    public:
        Client ();
        ~Client ();
        int start (char* serverIP, int port);
        void stop ();
};