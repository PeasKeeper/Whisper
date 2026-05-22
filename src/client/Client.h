#pragma once

#include <StopReason.h>

#include <string>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>

class Client {
    private:
        int sock;

        std::atomic<bool> running;

        StopReason stopReason;

        void verboseStop(StopReason stopReason);
        void printStopMessage() const;

        std::queue<std::string> outgoingMessages;
        std::mutex outgoingMutex;
        std::condition_variable outgoingCv;

        std::thread outputThread;
        std::thread recieveThread;

        void sendLoop();
        void recieveLoop();

        using UserMessageCallback =
            std::function<void(const std::string&, const std::string&)>;

        using SystemMessageCallback =
            std::function<void(const std::string&)>;

        UserMessageCallback onUserMessage;
        SystemMessageCallback onSystemMessage;

    public:
        Client ();
        ~Client () {};
        int start (char* serverIP, int port, std::string nickname);
        bool stop (StopReason reason);
        void joinThreads();

        void queueMessage(std::string message);

        void setUserMessageCallback(UserMessageCallback callback);
        void setSystemMessageCallback(SystemMessageCallback callback);
};
