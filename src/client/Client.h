#pragma once

#include <string>
#include <atomic>

class Client {
    private:
        int sock;

        std::atomic<bool> running;

        ssize_t sendMessage (const std::string& message) const;

    public:
        Client ();
        ~Client () {};
        int start (char* serverIP, int port, std::string nickname);
        void stop ();
};
