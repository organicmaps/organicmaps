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
  virtual ~PlatformSocket();
  virtual bool Open(string const & host, uint16_t port) override;
  virtual void Close() override;
  virtual bool Read(uint8_t * data, uint32_t count) override;
  virtual bool Write(uint8_t const * data, uint32_t count) override;
  virtual void SetTimeout(uint32_t milliseconds) override;
};

class MockSocket final : public Socket
{
public:
  virtual bool Open(string const & host, uint16_t port) override { return false; }
  virtual void Close() override {}

  virtual bool Read(uint8_t * data, uint32_t count) override { return false; }
  virtual bool Write(uint8_t const * data, uint32_t count) override { return false; }

  virtual void SetTimeout(uint32_t milliseconds) override {}
};

class TestSocket final : public Socket
{
public:
  TestSocket();
  virtual ~TestSocket();
  virtual bool Open(string const & host, uint16_t port) override;
  virtual void Close() override;
  virtual bool Read(uint8_t * data, uint32_t count) override;
  virtual bool Write(uint8_t const * data, uint32_t count) override;
  virtual void SetTimeout(uint32_t milliseconds) override;

  bool HasInput() const;
  bool HasOutput() const;

  // Simulate server writing
  void AddInput(uint8_t const * data, uint32_t count);

  // Simulate server reading
  // returns size of read data
  size_t FetchOutput(vector<uint8_t> & destination);

private:
  bool m_isConnected;

  vector<uint8_t> m_input;
  mutable mutex m_inputMutex;

  vector<uint8_t> m_output;
  mutable mutex m_outputMutex;
};

}  // namespace platform
