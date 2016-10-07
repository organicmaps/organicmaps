#pragma once

#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

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

class TestSocket : public Socket
{
public:
  virtual bool HasInput() const = 0;
  virtual bool HasOutput() const = 0;

  // Simulate server writing
  virtual void WriteServer(uint8_t const * data, uint32_t count) = 0;

  // Simulate server reading
  // returns size of read data
  virtual size_t ReadServer(vector<uint8_t> & destination) = 0;
};

unique_ptr<Socket> createSocket();
unique_ptr<Socket> createMockSocket();
unique_ptr<TestSocket> createTestSocket();
}  // namespace platform
