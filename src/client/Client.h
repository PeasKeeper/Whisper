#pragma once

#include <StopReason.h>

#include <string>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <thread>

class AppManager;

class Client {
    private:
        using UserMessageCallback = std::function<void(const std::string&, const std::string&)>;
        using SystemMessageCallback = std::function<void(const std::string&)>;

        int sock;
        std::atomic<bool> running;

        std::queue<std::string> outgoingMessages;
        std::mutex outgoingMutex;
        std::condition_variable outgoingCv;

        std::thread sendThread;
        std::thread receiveThread;
        void sendLoop();
        void receiveLoop();

        UserMessageCallback onUserMessage;
        SystemMessageCallback onSystemMessage;

        AppManager &appManager;

    public:
        Client (AppManager &newAppManager);
        ~Client ();

        StopReason connectToServer(char* serverIP, int port, std::string nickname);
        void run ();
        bool stop ();
        void joinThreads();

        void queueMessage(std::string message);

        void setUserMessageCallback(UserMessageCallback callback);
        void setSystemMessageCallback(SystemMessageCallback callback);
};
