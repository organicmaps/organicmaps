#pragma once

#include "std/mutex.hpp"
#include "std/string.hpp"
#include "std/target_os.hpp"
#include "std/vector.hpp"

namespace platform
{
class Socket
{
public:
  virtual ~Socket() {}
  virtual bool Open(string const & host, uint16_t port) = 0;
  virtual void Close() = 0;

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
