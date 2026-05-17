#include "Client.h"
#include "StopReason.h"

#include <SocketAdapter.h>
#include <StringUtils.h>

#include <iostream>
#include <thread>

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

int Client::start (char* serverIP, int port, string nickname) {
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

    auto verboseStop = [this](StopReason reason){
        if(stop(reason)) {
            printStopMessage();
        }
    };

    StopReason result = SocketAdapter::sendMessage(sock, StringUtils::stringToBytes(nickname));
    if (result != StopReason::None) {
        verboseStop(result);
        return -4;
    }

    thread inputThread = thread([&]{
        string message = "";
        while(running) {
            getline(cin, message, '\n');
            if (cin.eof()) {
                break;
            }
            if (message.empty()) {
                continue;
            }
            StopReason result = SocketAdapter::sendMessage(sock, StringUtils::stringToBytes(message));
            if (result != StopReason::None) {
                verboseStop(result);
                break;
            }
        }
    });

    cout <<  "\033[0;37mYou can list existing groups by typing /LSGRP\nYou can make a new group by typing /NEWGRP group_name password\nYou can join an existing group by typing /JOINGRP group_name password\nYou can leave a group by typing /LEAVEGRP \033[0m \n" << endl;

    while (running) {
        ReceiveResult result = SocketAdapter::receiveMessage(sock);
        if (result.stopReason != StopReason::None) {
            verboseStop(result.stopReason);
            break;
        }
        string currentMessage = StringUtils::bytesToString(result.payload);
        cout << currentMessage << endl;
    }

    cout << "\nPress enter to exit the application..." << endl;

    inputThread.join();

    return 0;
}

bool Client::stop(StopReason reason) {
    bool wasRunning = running.exchange(false);
    if (!wasRunning) {
        return false;
    }

    stopReason = reason;

    if (sock >= 0) {
        shutdown(sock, SHUT_RDWR);
        close(sock);
    }
    return true;
}

 void Client::printStopMessage() const {
     switch (stopReason) {
        case StopReason::None:
        case StopReason::LocalUser:
        case StopReason::NetworkError:
            break;

        case StopReason::Timeout:
            cout << "\nConnection timed out." << endl;
            break;

        case StopReason::ProtocolError:
            cout << "\nReceived invalid message." << endl;
            break;

        case StopReason::PeerClosed:
            cout << "\nServer shut down" << endl;
            break;
     }
 }
