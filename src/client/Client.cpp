#include "Client.h"

#include <iostream>
#include <string.h>
#include <unistd.h>
#include <thread>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <array>

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

    send(sock, reinterpret_cast<const void*>(nickname.data()), nickname.size()+1, 0);

    array<char, BUFFER_SIZE> buffer = {};

    thread inputThread = thread([&]{
        string message = "";
        while(running) {
            getline(cin, message, '\n');
            if (cin.eof()) {
                break;
            }
            if (message.size() > 0) {
                send(sock, reinterpret_cast<const void*>(message.data()), message.size()+1, 0);
            }
        }
    });

    while (running) {
        int bytes = recv(sock, buffer.data(), BUFFER_SIZE-1, 0);

        if (bytes > 0) {
            cout << buffer.data() << endl;
        }
        else if (!bytes){
            cout << "\nServer shut down" << endl;
            stop();
        }
        else {
            continue; // TODO: add error handling
        }
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
