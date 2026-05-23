#include "Client.h"
#include "StopReason.h"

#include <SocketAdapter.h>
#include <StringUtils.h>
#include <consts.h>

#include <iostream>
#include <thread>
#include <vector>

#include <cstring>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std;

Client::Client () {
    running = true;
    sock = -1;
}

int Client::start(char* serverIP, int port, string nickname) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket failed");
        return -1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, serverIP, &server_addr.sin_addr) <= 0) {
        perror("Socket failed");
        return -2;
    }

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        return -3;
    }

    StopReason result = SocketAdapter::sendMessage(sock, StringUtils::stringToBytes(nickname));
    if (result != StopReason::None) {
        verboseStop(result);
        return -4;
    }

    sendThread = thread(&Client::sendLoop, this);
    receiveThread = thread(&Client::receiveLoop, this);

    //cout << "\nPress enter to exit the application..." << endl;

    return 0;
}

bool Client::stop(StopReason reason) {
    bool wasRunning = running.exchange(false);
    if (!wasRunning) {
        return false;
    }

    stopReason = reason;
    outgoingCv.notify_all();

    if (sock >= 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
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
    {
        std::lock_guard<std::mutex> lock(outgoingMutex);
        outgoingMessages.push(std::move(message));
    }

    outgoingCv.notify_one();
}

void Client::setUserMessageCallback(UserMessageCallback callback) {
    onUserMessage = callback;
}

void Client::setSystemMessageCallback(SystemMessageCallback callback) {
    onSystemMessage = callback;
}

void Client::verboseStop(StopReason stopReason) {
    stop(stopReason);
    printStopMessage();
}

void Client::printStopMessage() const {
     switch (stopReason) {
        case StopReason::None:
        case StopReason::LocalUser:
            break;

        case StopReason::NetworkError:
            cout << "\nNetwork error." << endl;
            break;

        case StopReason::Timeout:
            cout << "\nConnection timed out." << endl;
            break;

        case StopReason::ProtocolError:
            cout << "\nReceived invalid message." << endl;
            break;

        case StopReason::PeerClosed:
            cout << "\nServer shut down." << endl;
            break;
     }
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
            stop(result);
            break;
        }
    }
 }

 void Client::receiveLoop() {
     while (running) {
         ReceiveResult result = SocketAdapter::receiveMessage(sock);
         if (result.stopReason != StopReason::None) {
             verboseStop(result.stopReason);
             break;
         }
         string currentMessage = StringUtils::bytesToString(result.payload);
         vector<string> messageParsed = StringUtils::splitString(currentMessage, MESSAGE_SEPARATOR);
         if (messageParsed.size() > 2) {
             verboseStop(StopReason::ProtocolError);
         }
         else if (messageParsed.size() > 1) {
             onUserMessage(messageParsed[0], messageParsed[1]);
         }
         else {
             onSystemMessage(messageParsed[0]);
         }
     }
 }
