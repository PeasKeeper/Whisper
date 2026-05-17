#include "SocketAdapter.h"
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
    array<unsigned char, FRAME_LENGTH_FIELD_SIZE> frameSizeBuf = {};
    size_t receivedBytes = 0;
    size_t currentMsgSize = 0;

    while (receivedBytes < FRAME_LENGTH_FIELD_SIZE) {
        ssize_t n = recv(sockFd, frameSizeBuf.data() + receivedBytes, FRAME_LENGTH_FIELD_SIZE - receivedBytes, 0);
        if (n < 0) {
            return {StopReason::NetworkError, {}};
        }
        if (n == 0) {
            return {StopReason::PeerClosed, {}};
        }
        receivedBytes += n;
    }

    uint32_t netLength = 0;
    std::memcpy(&netLength, frameSizeBuf.data(), FRAME_LENGTH_FIELD_SIZE);

    uint32_t length = ntohl(netLength);

    if (length == 0 || length > MAX_FRAME_SIZE) {
        return {StopReason::ProtocolError, {}};
    }

    vector<byte> currentPayload(length);
    while (currentMsgSize < length) {
        ssize_t n = recv(sockFd, currentPayload.data() + currentMsgSize, length - currentMsgSize, 0);
        if (n < 0) {
            return {StopReason::NetworkError, {}};
        }
        if (n == 0) {
            return {StopReason::PeerClosed, {}};
        }
        currentMsgSize += n;
    }

    return {StopReason::None, std::move(currentPayload)};
}
