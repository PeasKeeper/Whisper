#pragma once

#include <thread>
#include <string>

struct ClientData {
    std::thread clientThread;
    std::wstring nickname = L"";
};