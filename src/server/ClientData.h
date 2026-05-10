#pragma once

#include <thread>
#include <string>

struct ClientData {
    int socket = 0;
    std::thread clientThread;
    std::string nickname = "";
    std::string groupName = "";
};
