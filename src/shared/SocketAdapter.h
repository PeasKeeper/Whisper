#pragma once

#include "StopReason.h"

#include <vector>
#include <cstddef>

struct ReceiveResult {
    StopReason stopReason = StopReason::None;
    std::vector<std::byte> payload;
};

class SocketAdapter {
    public:
        static StopReason sendMessage(int sockFd, const std::vector<std::byte>& message);
        static ReceiveResult receiveMessage(int sockFd);
};
