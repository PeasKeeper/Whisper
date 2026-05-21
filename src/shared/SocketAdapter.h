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

    private:
        static bool waitReadable(int fd);
        static StopReason recvExactWithTimeout(int fd, std::byte* buffer, size_t size);
};
