#pragma once

#include <StopReason.h>

#include <string>
#include <atomic>

class Client {
    private:
        int sock;

        std::atomic<bool> running;

        StopReason stopReason;

        void printStopMessage() const;

    public:
        Client ();
        ~Client () {};
        int start (char* serverIP, int port, std::string nickname);
        bool stop (StopReason reason);
};
