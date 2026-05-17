#include "SocketAdapter.h"
#include "StopReason.h"
#include "consts.h"

#include <array>
#include <vector>
#include <utility>

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

using namespace std;

StopReason SocketAdapter::sendMessage(int sockFd, const vector<byte>& message) {
    size_t messageSize = message.size();
    if (messageSize > MAX_FRAME_SIZE) {
        return StopReason::ProtocolError;
    }

    uint32_t netLength = htonl(static_cast<uint32_t>(messageSize));

    std::vector<unsigned char> frame(FRAME_LENGTH_FIELD_SIZE + messageSize);

    std::memcpy(frame.data(), &netLength, FRAME_LENGTH_FIELD_SIZE);
    std::memcpy(frame.data() + FRAME_LENGTH_FIELD_SIZE, message.data(), messageSize);

    if (message.empty()) {
        return StopReason::ProtocolError;
    }

    size_t sentBytes = 0;
    while(sentBytes < frame.size()) {
        ssize_t n = send(sockFd, reinterpret_cast<const void*>(frame.data() + sentBytes), frame.size() - sentBytes, MSG_NOSIGNAL);
        if (n < 0) {
            return StopReason::NetworkError;
        }
        if (n == 0) {
            return StopReason::PeerClosed;
        }
        sentBytes += n;
    }
    return StopReason::None;
}

ReceiveResult SocketAdapter::receiveMessage(int sockFd) {
    array<byte, FRAME_LENGTH_FIELD_SIZE> frameSizeBuf = {};

    ssize_t n = recv(sockFd, frameSizeBuf.data(), 1, 0);
    if (n < 0) {
        return {StopReason::NetworkError, {}};
    }
    if (n == 0) {
        return {StopReason::PeerClosed, {}};
    }
    StopReason result = recvExactWithTimeout(sockFd, frameSizeBuf.data() + n, FRAME_LENGTH_FIELD_SIZE - n);
    if (result != StopReason::None) {
        return {result, {}};
    }

    uint32_t netLength = 0;
    std::memcpy(&netLength, frameSizeBuf.data(), FRAME_LENGTH_FIELD_SIZE);

    uint32_t length = ntohl(netLength);

    if (length == 0 || length > MAX_FRAME_SIZE) {
        return {StopReason::ProtocolError, {}};
    }

    vector<byte> currentPayload(length);
    result = recvExactWithTimeout(sockFd, currentPayload.data(), length);
    if (result != StopReason::None) {
        return {result, {}};
    }

    return {StopReason::None, std::move(currentPayload)};
}

bool SocketAdapter::waitReadable(int fd) {
    pollfd pfd{};
    pfd.fd = fd;
    pfd.events = POLLIN;

    int result = poll(&pfd, 1, FRAME_TIMEOUT_MS);
    return result > 0 && (pfd.revents & POLLIN);
}

StopReason SocketAdapter::recvExactWithTimeout(int fd, byte* buffer, size_t size) {
    size_t receivedBytes = 0;
    while (receivedBytes < size) {
        if (!waitReadable(fd)) {
            return StopReason::Timeout;
        }

        ssize_t n = recv(fd, buffer + receivedBytes, size - receivedBytes, 0);
        if (n == 0) {
            return StopReason::PeerClosed;
        }
        if (n < 0) {
            return StopReason::NetworkError;
        }
        receivedBytes += static_cast<size_t>(n);
    }

    return StopReason::None;
}
