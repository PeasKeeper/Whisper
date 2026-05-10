#pragma once

#include "ClientData.h"

#include <string>
#include <unordered_map>

struct Group {
    std::unordered_map<int, ClientData&> users;
    std::string groupName = "";
    std::string password = "";
};
