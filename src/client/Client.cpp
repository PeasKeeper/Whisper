#include "Client.h"
#include "AppManager.h"

#include <SocketAdapter.h>
#include <StringUtils.h>
#include <consts.h>

#include <mutex>
#include <vector>

using namespace std;

Client::Client (AppManager &newAppManager) : appManager(newAppManager) {
    running = true;
    sock = -1;
}

Client::~Client () {
    stop();
    joinThreads();
}

StopReason Client::connectToServer(char* serverIP, int port, std::string nickname) {
    sock = SocketAdapter::openSocket();
    if (sock == -1) {
        return StopReason::NetworkError;
    }

    StopReason result = SocketAdapter::connectToAddress(sock, serverIP, port);
    if (result != StopReason::None) {
        return result;
    }

    result = SocketAdapter::sendMessage(sock, StringUtils::stringToBytes(nickname));
    if (result != StopReason::None) {
        return result;
    }
    return StopReason::None;
}

void Client::run() {
    sendThread = thread(&Client::sendLoop, this);
    receiveThread = thread(&Client::receiveLoop, this);
}

bool Client::stop() {
    bool wasRunning = running.exchange(false);
    if (!wasRunning) {
        return false;
    }

    outgoingCv.notify_all();

    if (sock >= 0) {
        SocketAdapter::closeSocket(sock);
        sock = -1;
    }
    return true;
}

void Client::joinThreads() {
    if (sendThread.joinable()) {
        sendThread.join();
    }

    if (receiveThread.joinable()) {
        receiveThread.join();
    }
}

void Client::queueMessage(std::string message) {
    std::unique_lock<std::mutex> lock(outgoingMutex);
    outgoingMessages.push(std::move(message));
    lock.unlock();

    outgoingCv.notify_one();
}

void Client::setUserMessageCallback(UserMessageCallback callback) {
    onUserMessage = callback;
}

void Client::setSystemMessageCallback(SystemMessageCallback callback) {
    onSystemMessage = callback;
}

void Client::sendLoop() {
    while (running) {
        std::string message;
        std::unique_lock<std::mutex> lock(outgoingMutex);

        outgoingCv.wait(lock, [&] {
            return !running || !outgoingMessages.empty();
        });
        if (!running && outgoingMessages.empty()) {
            break;
        }

        message = std::move(outgoingMessages.front());
        outgoingMessages.pop();

        lock.unlock();

        StopReason result = SocketAdapter::sendMessage(sock, StringUtils::stringToBytes(message));
        if (result != StopReason::None) {
            appManager.requestStop(result);
            break;
        }
    }
}

void Client::receiveLoop() {
    while (running) {
        ReceiveResult result = SocketAdapter::receiveMessage(sock);
        if (result.stopReason != StopReason::None) {
            appManager.requestStop(result.stopReason);
            break;
        }
        string currentMessage = StringUtils::bytesToString(result.payload);
        vector<string> messageParsed = StringUtils::splitString(currentMessage, MESSAGE_SEPARATOR);
        if (messageParsed.size() > 2) {
            appManager.requestStop(StopReason::ProtocolError);
        }
        else if (messageParsed.size() > 1) {
            if (onUserMessage) {
                onUserMessage(messageParsed[0], messageParsed[1]);
            }
        }
        else {
            if (onSystemMessage) {
                onSystemMessage(messageParsed[0]);
            }
        }
    }
}
