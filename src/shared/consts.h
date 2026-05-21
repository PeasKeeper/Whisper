#pragma once

#include <cstddef>
#include <cstdint>

constexpr std::size_t MAX_FRAME_SIZE = 8ull * 1024ull * 1024ull; // 8MiB
constexpr std::size_t FRAME_LENGTH_FIELD_SIZE = sizeof(std::uint32_t);

constexpr int FRAME_TIMEOUT_MS = 30'000;
