#pragma once

#include "platform/socket.hpp"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <iterator>
#include <mutex>
#include <string>
#include <vector>

namespace platform
{
namespace tests_support
{
class TestSocket final : public Socket
{
public:
  // Socket overrides:
  ~TestSocket();
  bool Open(std::string const & host, uint16_t port) override;
  void Close() override;
  bool Read(uint8_t * data, uint32_t count) override;
  bool Write(uint8_t const * data, uint32_t count) override;
  void SetTimeout(uint32_t milliseconds) override;

  // Simulates server reading.
  // Waits for some data or timeout.
  // Returns size of read data.
  size_t ReadServer(std::vector<uint8_t> & destination);
  template <typename Container>
  void WriteServer(Container const & answer)
  {
    std::lock_guard<std::mutex> lg(m_inputMutex);
    m_input.insert(m_input.begin(), std::begin(answer), std::end(answer));
    m_inputCondition.notify_one();
  }

private:
  std::atomic<bool> m_isConnected = {false};
  std::atomic<uint32_t> m_timeoutMs = {100};

  std::deque<uint8_t> m_input;
  std::mutex m_inputMutex;
  std::condition_variable m_inputCondition;

  std::vector<uint8_t> m_output;
  std::mutex m_outputMutex;
  std::condition_variable m_outputCondition;
};
}  // namespace tests_support
}  // namespace platform
