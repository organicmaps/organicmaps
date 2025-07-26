#include "test_socket.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <chrono>

namespace platform
{
namespace tests_support
{
TestSocket::~TestSocket()
{
  m_isConnected = false;
}

bool TestSocket::Open(std::string const & host, uint16_t port)
{
  if (m_isConnected)
    return false;

  m_isConnected = true;
  return true;
}

void TestSocket::Close()
{
  m_isConnected = false;
}

bool TestSocket::Read(uint8_t * data, uint32_t count)
{
  if (!m_isConnected)
    return false;

  std::unique_lock<std::mutex> lock(m_inputMutex);

  m_inputCondition.wait_for(lock, std::chrono::milliseconds(m_timeoutMs), [this]() { return !m_input.empty(); });
  if (m_input.size() < count)
    return false;

  std::copy(m_input.begin(), m_input.end(), data);
  m_input.erase(m_input.begin(), m_input.begin() + count);
  return true;
}

bool TestSocket::Write(uint8_t const * data, uint32_t count)
{
  if (!m_isConnected)
    return false;

  {
    std::lock_guard lg(m_outputMutex);
    m_output.insert(m_output.end(), data, data + count);
  }
  m_outputCondition.notify_one();
  return true;
}

void TestSocket::SetTimeout(uint32_t milliseconds)
{
  m_timeoutMs = milliseconds;
}
size_t TestSocket::ReadServer(std::vector<uint8_t> & destination)
{
  std::unique_lock lock(m_outputMutex);
  m_outputCondition.wait_for(lock, std::chrono::milliseconds(m_timeoutMs), [this]() { return !m_output.empty(); });

  size_t const outputSize = m_output.size();
  destination.insert(destination.end(), m_output.begin(), m_output.end());
  m_output.clear();
  return outputSize;
}
}  // namespace tests_support
}  // namespace platform
