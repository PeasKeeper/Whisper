#pragma once

enum class StopReason {
    None = 0,
    LocalUser,
    ServerClosed,
    NetworkError,
    ProtocolError
};
