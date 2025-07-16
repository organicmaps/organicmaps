#pragma once

#include <cstdint>

constexpr uint32_t READER_CHUNK_LOG_SIZE = 10;  // 1024 bytes pages
constexpr uint32_t READER_CHUNK_LOG_COUNT =
    12;  // 2^12 = 4096 pages count, hence 4M size cache overall (for data files only)
