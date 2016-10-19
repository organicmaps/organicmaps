#pragma once

#include "platform/socket.hpp"

#include "std/atomic.hpp"
#include "std/condition_variable.hpp"
#include "std/cstdint.hpp"
#include "std/deque.hpp"
#include "std/mutex.hpp"
#include "std/vector.hpp"

namespace platform
{
namespace tests_support
{
class TestSocket final : public Socket
{
public:
  // Socket overrides:
  ~TestSocket();
  bool Open(string const & host, uint16_t port) override;
  void Close() override;
  bool Read(uint8_t * data, uint32_t count) override;
  bool Write(uint8_t const * data, uint32_t count) override;
  void SetTimeout(uint32_t milliseconds) override;

  // Simulates server reading.
  // Waits for some data or timeout.
  // Returns size of read data.
  size_t ReadServer(vector<uint8_t> & destination);

private:
  atomic<bool> m_isConnected = {false};
  atomic<uint32_t> m_timeoutMs = {100};

  deque<uint8_t> m_input;
  mutex m_inputMutex;

  vector<uint8_t> m_output;
  mutex m_outputMutex;
  condition_variable m_outputCondition;
};
}  // namespace tests_support
}  // namespace platform
