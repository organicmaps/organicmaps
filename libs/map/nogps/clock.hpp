#pragma once

#include <cstdint>

namespace nogps
{
class Clock
{
public:
  virtual ~Clock() = default;

  /// \returns monotonic milliseconds that go on while the device sleeps, the time base of all the positions and
  /// sensor events: SystemClock.elapsedRealtime() on Android.
  virtual int64_t NowMs() const = 0;

  /// \returns milliseconds since the Unix epoch.
  virtual int64_t UnixNowMs() const = 0;
};

class SystemClock : public Clock
{
public:
  int64_t NowMs() const override;
  int64_t UnixNowMs() const override;
};
}  // namespace nogps
