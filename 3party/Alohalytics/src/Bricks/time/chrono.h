// This file can not be named `time.h`, since it would interfere with C/C++ standard header.

#ifndef BRICKS_TIME_CHRONO_H
#define BRICKS_TIME_CHRONO_H

#include <chrono>

#include "../strings/fixed_size_serializer.h"

// TODO(dkorolev): Add platform-dependent tests comparing Bricks time to UNIX time.

namespace bricks {

namespace time {

// enum class EPOCH_MILLISECONDS : uint64_t {};
typedef uint64_t EPOCH_MILLISECONDS;
// enum class MILLISECONDS_INTERVAL : uint64_t {};
typedef uint64_t MILLISECONDS_INTERVAL;

#ifndef BRICKS_ANDROID

// Since chrono::system_clock is not monotonic, and chrono::steady_clock is not guaranteed to be Epoch,
// use a simple wrapper around chrono::system_clock to make it non-decreasing.
struct EpochClockGuaranteeingMonotonicity {
  struct Impl {
    mutable uint64_t monotonic_now = 0ull;
    inline uint64_t Now() const {
      const uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch()).count();
      monotonic_now = std::max(monotonic_now, now);
      return monotonic_now;
    }
  };
  static const Impl& Singleton() {
    static Impl singleton;
    return singleton;
  }
};

inline EPOCH_MILLISECONDS Now() {
  return static_cast<EPOCH_MILLISECONDS>(EpochClockGuaranteeingMonotonicity::Singleton().Now());
}

#else

// Android C++ compiler doesn't support `thread_local`, fall back to the naive implementation.
inline EPOCH_MILLISECONDS Now() {
  return static_cast<EPOCH_MILLISECONDS>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                             std::chrono::system_clock::now().time_since_epoch()).count());
}

#endif

}  // namespace time

namespace strings {

template <>
struct FixedSizeSerializer<bricks::time::EPOCH_MILLISECONDS> {
  enum { size_in_bytes = std::numeric_limits<uint64_t>::digits10 + 1 };
  static std::string PackToString(bricks::time::EPOCH_MILLISECONDS x) {
    std::ostringstream os;
    os << std::setfill('0') << std::setw(size_in_bytes) << static_cast<uint64_t>(x);
    return os.str();
  }
  static bricks::time::EPOCH_MILLISECONDS UnpackFromString(std::string const& s) {
    uint64_t x;
    std::istringstream is(s);
    is >> x;
    return static_cast<bricks::time::EPOCH_MILLISECONDS>(x);
  }
};

}  // namespace strings

}  // namespace bricks

// inline bricks::time::MILLISECONDS_INTERVAL operator-(bricks::time::EPOCH_MILLISECONDS lhs,
//                                                     bricks::time::EPOCH_MILLISECONDS rhs) {
//  return static_cast<bricks::time::MILLISECONDS_INTERVAL>(static_cast<int64_t>(lhs) -
//                                                          static_cast<int64_t>(rhs));
//}

// inline bricks::time::MILLISECONDS_INTERVAL operator-(bricks::time::EPOCH_MILLISECONDS x) {
//  return static_cast<bricks::time::MILLISECONDS_INTERVAL>(-static_cast<int64_t>(x));
//}

// inline bricks::time::EPOCH_MILLISECONDS operator+(bricks::time::EPOCH_MILLISECONDS lhs,
//                                                  bricks::time::MILLISECONDS_INTERVAL rhs) {
//  return static_cast<bricks::time::EPOCH_MILLISECONDS>(static_cast<int64_t>(lhs) + static_cast<int64_t>(rhs));
//}

// inline bricks::time::EPOCH_MILLISECONDS operator-(bricks::time::EPOCH_MILLISECONDS lhs,
//                                                  bricks::time::MILLISECONDS_INTERVAL rhs) {
//  return static_cast<bricks::time::EPOCH_MILLISECONDS>(static_cast<int64_t>(lhs) - static_cast<int64_t>(rhs));
//}

// inline bricks::time::EPOCH_MILLISECONDS operator+(bricks::time::MILLISECONDS_INTERVAL lhs,
//                                                  bricks::time::EPOCH_MILLISECONDS rhs) {
//  return static_cast<bricks::time::EPOCH_MILLISECONDS>(static_cast<int64_t>(lhs) + static_cast<int64_t>(rhs));
//}

// TODO(dkorolev): Add more arithmetic operations on milliseconds here.

#endif
