#include "socket.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"

namespace platform {

TestSocket::TestSocket()
  : m_isConnected(false)
{
}

TestSocket::~TestSocket()
{
  m_isConnected = false;
}

bool TestSocket::Open(string const & host, uint16_t port)
{
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

  lock_guard<mutex> lg(m_inputMutex);

  if (m_input.size() < count)
    return false;

  copy( m_input.data(), m_input.data()+count, data );
  m_input.erase( m_input.begin(), m_input.begin() + count );
  return true;
}

bool TestSocket::Write(uint8_t const * data, uint32_t count)
{
  if (!m_isConnected)
    return false;

  lock_guard<mutex> lg(m_outputMutex);
  m_output.insert(m_output.end(), data, data + count);
  return true;
}

void TestSocket::SetTimeout(uint32_t milliseconds)
{
}

bool TestSocket::HasInput() const
{
  lock_guard<mutex> lg(m_inputMutex);
  return !m_input.empty();
}

bool TestSocket::HasOutput() const
{
  lock_guard<mutex> lg(m_outputMutex);
  return !m_output.empty();
}

void TestSocket::AddInput(uint8_t const * data, uint32_t count)
{
  ASSERT(m_isConnected, ());

  lock_guard<mutex> lg(m_inputMutex);
  m_input.insert(m_input.end(), data, data + count);
}

size_t TestSocket::FetchOutput(vector<uint8_t> & destination)
{
  lock_guard<mutex> lg(m_outputMutex);

  size_t const outputSize = m_output.size();
  if (outputSize == 0)
    return 0;

  destination.insert(destination.end(), m_output.begin(), m_output.end());
  m_output.clear();
  return outputSize;
}
} // namespace platform
