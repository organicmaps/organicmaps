#include "socket.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/deque.hpp"
#include "std/mutex.hpp"

namespace
{
class MockSocket final : public platform::Socket
{
public:
  // Socket overrides
  bool Open(string const & host, uint16_t port) override { return false; }
  void Close() override {}
  bool Read(uint8_t * data, uint32_t count) override { return false; }
  bool Write(uint8_t const * data, uint32_t count) override { return false; }
  void SetTimeout(uint32_t milliseconds) override {}
};

class TestSocketImpl final : public platform::TestSocket
{
public:
  // Socket overrides
  ~TestSocketImpl();
  bool Open(string const & host, uint16_t port) override;
  void Close() override;
  bool Read(uint8_t * data, uint32_t count) override;
  bool Write(uint8_t const * data, uint32_t count) override;
  void SetTimeout(uint32_t milliseconds) override;

  // TestSocket overrides
  bool HasInput() const override;
  bool HasOutput() const override;
  void WriteServer(uint8_t const * data, uint32_t count) override;
  size_t ReadServer(vector<uint8_t> & destination) override;

private:
  bool m_isConnected = false;

  deque<uint8_t> m_input;
  mutable mutex m_inputMutex;

  vector<uint8_t> m_output;
  mutable mutex m_outputMutex;
};

TestSocketImpl::~TestSocketImpl() { m_isConnected = false; }

bool TestSocketImpl::Open(string const & host, uint16_t port)
{
  m_isConnected = true;
  return true;
}

void TestSocketImpl::Close() { m_isConnected = false; }
bool TestSocketImpl::Read(uint8_t * data, uint32_t count)
{
  if (!m_isConnected)
    return false;

  lock_guard<mutex> lg(m_inputMutex);

  if (m_input.size() < count)
    return false;

  copy(m_input.begin(), m_input.end(), data);
  m_input.erase(m_input.begin(), m_input.begin() + count);
  return true;
}

bool TestSocketImpl::Write(uint8_t const * data, uint32_t count)
{
  if (!m_isConnected)
    return false;

  lock_guard<mutex> lg(m_outputMutex);
  m_output.insert(m_output.end(), data, data + count);
  return true;
}

void TestSocketImpl::SetTimeout(uint32_t milliseconds) {}
bool TestSocketImpl::HasInput() const
{
  lock_guard<mutex> lg(m_inputMutex);
  return !m_input.empty();
}

bool TestSocketImpl::HasOutput() const
{
  lock_guard<mutex> lg(m_outputMutex);
  return !m_output.empty();
}

void TestSocketImpl::WriteServer(uint8_t const * data, uint32_t count)
{
  ASSERT(m_isConnected, ());

  lock_guard<mutex> lg(m_inputMutex);
  m_input.insert(m_input.end(), data, data + count);
}

size_t TestSocketImpl::ReadServer(vector<uint8_t> & destination)
{
  lock_guard<mutex> lg(m_outputMutex);

  size_t const outputSize = m_output.size();
  if (outputSize == 0)
    return 0;

  destination.insert(destination.end(), m_output.begin(), m_output.end());
  m_output.clear();
  return outputSize;
}
}  // namespace

namespace platform
{
unique_ptr<Socket> createMockSocket() { return make_unique<MockSocket>(); }
unique_ptr<TestSocket> createTestSocket() { return make_unique<TestSocketImpl>(); }
}  // namespace platform
