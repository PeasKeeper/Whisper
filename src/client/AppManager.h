#pragma once

#include "Client.h"
#include "UiManager.h"

#include <StopReason.h>

#include <mutex>
#include <string>
#include <thread>

class AppManager {
    private:
        char* serverIp;
        int serverPort;
        std::string userNickname;

        bool stopRequested;
        bool fullStopped;

        StopReason stopReason;
        std::mutex stopMutex;

        Client client;
        ftxui::UiManager uiManager;

        std::thread::id mainThreadId;

        void stopImpl(StopReason reason);

    public:
        AppManager(char* newServerIp, int newServerPort, std::string newUserNickname);
        ~AppManager();

        void run();
        void requestStop(StopReason reason);
        void finalize();
        void printStopMessage() const;
};
