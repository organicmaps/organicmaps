#pragma once

#include "std/string.hpp"
#include "std/target_os.hpp"

namespace platform
{
class Socket
{
public:
  virtual ~Socket() = default;

  // Open/Close contract:
  // 1. You can call Open+Close pair multiple times for the same Socket instance.
  // 2. There are should be Close call after each Open call.
  // 3. Open+Open: second Open does nothing and returns false.
  // 4. Close+Close: second Close does nothing.
  virtual bool Open(string const & host, uint16_t port) = 0;
  virtual void Close() = 0;

  // Read is blocking, it waits for the 'count' data size.
  virtual bool Read(uint8_t * data, uint32_t count) = 0;
  virtual bool Write(uint8_t const * data, uint32_t count) = 0;

  virtual void SetTimeout(uint32_t milliseconds) = 0;
};

class PlatformSocket final : public Socket
{
  PlatformSocket();
  // Socket overrides
  ~PlatformSocket();
  bool Open(string const & host, uint16_t port) override;
  void Close() override;
  bool Read(uint8_t * data, uint32_t count) override;
  bool Write(uint8_t const * data, uint32_t count) override;
  void SetTimeout(uint32_t milliseconds) override;
};
}  // namespace platform
