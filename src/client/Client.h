#pragma once

#include <string>
#include <atomic>

constexpr int BUFFER_SIZE = 4096;

class Client {
    private:
        int sock;

        std::atomic<bool> running;

    public:
        Client ();
        ~Client () {};
        int start (char* serverIP, int port, std::string nickname);
        void stop ();
};
