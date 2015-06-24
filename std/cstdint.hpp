#pragma once

#ifdef new
#undef new
#endif

#include <cstdint>
#include <cstddef>

using std::int8_t;
using std::uint8_t;
using std::int16_t;
using std::uint16_t;
using std::int32_t;
using std::uint32_t;
using std::int64_t;
using std::uint64_t;
using std::size_t;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
