#include "Client.h"

#include <consts.h>

#include <cstdint>
#include <iostream>
#include <sys/types.h>
#include <thread>
#include <array>
#include <vector>

#include <cstring>

#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

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

    if (sendMessage(nickname) <= 0) {
        cout << "Failed to send nickname.";
        stop();
        return -4;
    }

    thread inputThread = thread([&]{
        string message = "";
        while(running) {
            getline(cin, message, '\n');
            if (cin.eof()) {
                break;
            }
            if (sendMessage(message) < 0) {
                cout << "Failed to send message." << endl;
            }
        }
    });

    cout <<  "\033[0;37mYou can list existing groups by typing /LSGRP\nYou can make a new group by typing /NEWGRP group_name password\nYou can join an existing group by typing /JOINGRP group_name password\nYou can leave a group by typing /LEAVEGRP \033[0m \n" << endl;

    array<unsigned char, FRAME_LENGTH_FIELD_SIZE> frameSizeBuf = {};

    while (running) {
        size_t receivedBytes = 0;
        size_t currentMsgSize = 0;

        while (receivedBytes < FRAME_LENGTH_FIELD_SIZE) {
            ssize_t n = recv(sock, frameSizeBuf.data() + receivedBytes, FRAME_LENGTH_FIELD_SIZE - receivedBytes, 0);
            if (n <= 0) {
                cout << "\nServer shut down" << endl;
                stop();
                break;
            }
            receivedBytes += n;
        }

        if (!running) {
            break;
        }

        uint32_t netLength = 0;
        std::memcpy(&netLength, frameSizeBuf.data(), FRAME_LENGTH_FIELD_SIZE);

        uint32_t length = ntohl(netLength);

        string currentMessage = "";
        if (length > MAX_FRAME_SIZE) {
            cout << "\nReceived invalid message." << endl;
            stop();
            break;
        }

        currentMessage.resize(length);

        while (currentMsgSize < length) {
            ssize_t n = recv(sock, currentMessage.data() + currentMsgSize, length - currentMsgSize, 0);
            if (n <= 0) {
                cout << "\nServer shut down" << endl;
                stop();
                break;
            }
            currentMsgSize += n;
        }
        if (!running) {
            break;
        }
        cout << currentMessage << endl;
    }

    inputThread.join();

    return 0;
}

void Client::stop() {
    running = false;
    if (sock >= 0) {
        close(sock);
    }
    cout << "\nPress enter to exit the application..." << endl;
}

 ssize_t Client::sendMessage (const std::string& message) const {
     if (message.empty()) {
         return 0;
     }

     size_t messageSize = message.size();
     if (messageSize > MAX_FRAME_SIZE) {
         return -1;
     }

     uint32_t netLength = htonl(static_cast<uint32_t>(messageSize));

     std::vector<unsigned char> frame(FRAME_LENGTH_FIELD_SIZE + messageSize);

     std::memcpy(frame.data(), &netLength, FRAME_LENGTH_FIELD_SIZE);
     std::memcpy(frame.data() + FRAME_LENGTH_FIELD_SIZE, message.data(), messageSize);

     size_t sentBytes = 0;
     while(sentBytes < frame.size()) {
         ssize_t n = send(sock, reinterpret_cast<const void*>(frame.data() + sentBytes), frame.size() - sentBytes, 0);
         if (n <= 0) {
             return -1;
         }
         sentBytes += n;
     }
     return sentBytes;
 }
