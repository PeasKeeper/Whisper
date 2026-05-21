#pragma once

enum class StopReason {
    None = 0,
    LocalUser,
    PeerClosed,
    NetworkError,
    ProtocolError,
    Timeout
};
