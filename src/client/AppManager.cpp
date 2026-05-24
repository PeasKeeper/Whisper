#include "AppManager.h"

#include "StopReason.h"
#include "UiManager.h"

#include <iostream>

#include <cassert>

using namespace std;

AppManager::AppManager(char* newServerIp, int newServerPort, std::string newUserNickname)
    : serverIp(newServerIp),
      serverPort(newServerPort),
      userNickname(std::move(newUserNickname)),
      client(*this),
      uiManager(*this)
{
    stopReason = StopReason::None;
    stopRequested = false;
    finalized = false;
    mainThreadId = std::this_thread::get_id();
}

AppManager::~AppManager() {
    stopImpl(StopReason::LocalUser);
    finalize();
}

void AppManager::run() {
    client.setUserMessageCallback([this](std::string author, std::string body) {
        uiManager.postUserMessage(std::move(author), std::move(body));
    });

    client.setSystemMessageCallback([this](std::string body) {
        uiManager.postSystemMessage(std::move(body));
    });

    uiManager.setSendMessageCallback([this](std::string message) {
        client.queueMessage(std::move(message));
    });

    StopReason reason = client.connectToServer(serverIp, serverPort, userNickname);
    if (reason != StopReason::None) {
        stopImpl(reason);
    }
    else {
        client.run();
        uiManager.run();
    }
    stopImpl(StopReason::LocalUser);
}

void AppManager::requestStop(StopReason reason) {
    stopImpl(reason);
}

void AppManager::finalize() {
    assert(stopRequested);
    assert(std::this_thread::get_id() == mainThreadId);
    std::unique_lock<mutex> lock(stopMutex);
    if (finalized) {
        return;
    }
    finalized = true;
    lock.unlock();

    client.joinThreads();
}

void AppManager::printStopMessage() const {
    switch (stopReason) {
       case StopReason::None:
       case StopReason::LocalUser:
           break;

       case StopReason::NetworkError:
           cout << "Network error." << endl;
           break;

       case StopReason::Timeout:
           cout << "Connection timed out." << endl;
           break;

       case StopReason::ProtocolError:
           cout << "Received invalid message." << endl;
           break;

       case StopReason::PeerClosed:
           cout << "Server shut down." << endl;
           break;
    }
}

void AppManager::stopImpl(StopReason reason) {
    std::unique_lock<mutex> lock(stopMutex);
    if (stopRequested) {
        return;
    }
    stopRequested = true;
    stopReason = reason;
    lock.unlock();

    client.stop();
    uiManager.stop();
}
