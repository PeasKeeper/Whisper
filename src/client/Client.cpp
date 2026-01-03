#include "Client.h"

#include <wchar.h>
#include <string.h>
#include <unistd.h>
#include <thread>
#include <arpa/inet.h>
#include <netinet/in.h>

using namespace std;

Client::Client () {
    running = true;
    sock = -1;
}

int Client::start (char* serverIP, int port, wstring nickname) {
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

    send(sock, reinterpret_cast<const void*>(nickname.data()), (nickname.size()+1) * sizeof(wchar_t), 0);

    wstring message = L"";
    wchar_t buffer[BUFFER_SIZE] = {0};

    thread inputThread = thread([&]{
        while(running) {
            getline(wcin, message, L'\n');
            if (wcin.eof()) {
                break;
            }
            if (message.size() > 0) {
                send(sock, reinterpret_cast<const void*>(message.data()), (message.size()+1) * sizeof(wchar_t), 0);
            }
        }
    });

    while (running) {
        int bytes = recv(sock, buffer, (BUFFER_SIZE-1)*sizeof(wchar_t), 0);

        if (bytes > 0) {
            wcout << buffer << endl;
        }

        else if (!bytes){
            wcout << L"\nServer shut down" << endl;
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
    wcout << L"\nPress enter to exit the application..." << endl;
    //close(STDIN_FILENO); //closing wcin to join input thread
}