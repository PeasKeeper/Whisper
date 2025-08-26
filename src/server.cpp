#include "server.h"

using namespace std;

static mutex clientMutex;

wstring getHelpMsg() {
    return L"Usage: server [options]\n\nOptions:\n  --help        Display this help message and exit\n  -p <port>     Set the server port manually (default port is used if not specified)";
}

void getClientMessage (const int client_fd, set<int>* clients) {

    wchar_t buffer[BUFFER_SIZE] = {0};

    while (true) {
        if (recv(client_fd, buffer, BUFFER_SIZE*sizeof(wchar_t), 0)) { //TODO: consider windows compatibility 

            lock_guard<mutex> lock(clientMutex);

            for (auto client = clients->begin(); client != clients->end(); ++client) {
                if (*client != client_fd) {
                    send(*client, static_cast<const void*>(buffer), BUFFER_SIZE*sizeof(wchar_t), 0);
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");

    int port = 3418;

    if (argc == 2) {
        if (!strcmp(argv[1], "--help")) {
            wcout << getHelpMsg() << endl;
            return 0;
        }
        else { // for potential future flags
            wcout << getHelpMsg() << endl;
            return 0;
        }
    }
    else if (argc == 3) {
        if (!strcmp(argv[1], "-p")) {
            port = atoi(argv[2]);
        }
        else {
            wcout << getHelpMsg() << endl;
            return 0;
        }
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket failed");
        return -1;
    }

    sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return -2;
    }

    if (listen(server_fd, 1) < 0) {
        perror("Listen failed");
        close(server_fd);
        return -3;
    }

    wcout << L"Server up on " << getLocalIPv4() << L":" << port << endl;

    set<int>* clients = new set<int>;
    vector<thread>* threads = new vector <thread>;

    while (true) {

        int client_fd = accept(server_fd, NULL, NULL);

        unique_lock<mutex> lock(clientMutex);
        
        auto setCheck = clients->insert(client_fd);

        lock.unlock();

        if (client_fd < 0 || !setCheck.second) {
            perror("Accept failed");
            close(server_fd);
            return -4;
        }

        wcout << L"New user connected" << endl;
        
        thread t(getClientMessage, client_fd, clients);
        threads->push_back(move(t));
    }

    for (auto client : *clients) {
        close(client);
    }
    close(server_fd);

    for (auto &thread : *threads) {
        thread.join();
    }
    
    delete clients;
    delete threads;

    cout << L"Server shut down" << endl;
    return 0;
}