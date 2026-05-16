#pragma once

#include <StopReason.h>

#include <string>
#include <atomic>

#include <sys/types.h>

class Client {
    private:
        int sock;

        std::atomic<bool> running;

        ssize_t sendMessage (const std::string& message) const;

        StopReason stopReason;
        void printStopMessage () const;

    public:
        Client ();
        ~Client () {};
        int start (char* serverIP, int port, std::string nickname);
        void stop (StopReason reason);
};
