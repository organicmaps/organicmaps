#pragma once

#include <cstdint>
#include <memory>
#include <string>

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
  virtual bool Open(std::string const & host, uint16_t port) = 0;
  virtual void Close() = 0;

  // Read is blocking, it waits for the 'count' data size.
  virtual bool Read(uint8_t * data, uint32_t count) = 0;
  virtual bool Write(uint8_t const * data, uint32_t count) = 0;

  virtual void SetTimeout(uint32_t milliseconds) = 0;
};

class StubSocket final : public Socket
{
public:
  // Socket overrides:
  bool Open(std::string const &, uint16_t) override { return false; }
  void Close() override {}
  bool Read(uint8_t *, uint32_t) override { return false; }
  bool Write(uint8_t const *, uint32_t) override { return false; }
  void SetTimeout(uint32_t) override {}
};

std::unique_ptr<Socket> CreateSocket();
}  // namespace platform
