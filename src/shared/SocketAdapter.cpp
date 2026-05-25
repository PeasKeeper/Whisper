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

int SocketAdapter::openSocket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    return sock;
}

void SocketAdapter::closeSocket(int sockFd) {
    if (sockFd >= 0) {
        shutdown(sockFd, SHUT_RDWR);
        close(sockFd);
    }
}

StopReason SocketAdapter::connectToAddress(int sockFd, const char* ip, int port) {
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        return StopReason::NetworkError;
    }

    if (connect(sockFd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        return StopReason::NetworkError;
    }
    return StopReason::None;
}

StopReason SocketAdapter::listenOnSocket(int sockFd, int port, int backlog) {
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    int opt = 1;
    if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        return StopReason::NetworkError;
    }

    if (bind(sockFd, (sockaddr*)&address, sizeof(address)) < 0) {
        return StopReason::NetworkError;
    }

    if (listen(sockFd, backlog) < 0) {
        return StopReason::NetworkError;
    }
    return StopReason::None;
}

int SocketAdapter::acceptConnection(int sockFd) {
    int newFd = accept(sockFd, nullptr, nullptr);
    return newFd;
}

StopReason SocketAdapter::sendMessage(int sockFd, const std::vector<std::byte>& message) {
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
    std::array<std::byte, FRAME_LENGTH_FIELD_SIZE> frameSizeBuf = {};

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

    std::vector<std::byte> currentPayload(length);
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

StopReason SocketAdapter::recvExactWithTimeout(int fd, std::byte* buffer, size_t size) {
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
