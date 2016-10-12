#include "../core/jni_helper.hpp"
#include "platform/socket.hpp"
#include "base/logging.hpp"


/*class SocketImpl
{
public:
  SocketImpl()
  {
    // create m_self
  }

  ~SocketImpl()
  {
    // close socket
    // delete m_self
  }

  bool Open(string const & host, uint16_t port) { return false; }

  bool Close() { return false; }

  bool Read(uint8_t * data, uint32_t count) { return false; }

  bool Write(uint8_t const * data, uint32_t count) { return false; }

  void SetTimeout(uint32_t milliseconds) { };

private:
  jobject m_self;
};

namespace platform
{
Socket::Socket() { m_socketImpl = new SocketImpl(); }

Socket::~Socket() { delete m_socketImpl; }

bool Socket::Open(string const & host, uint16_t port) { return m_socketImpl->Open(host, port); }

void Socket::Close() { m_socketImpl->Close(); }

bool Socket::Read(uint8_t * data, uint32_t count) { return m_socketImpl->Read(data, count); }

bool Socket::Write(uint8_t const * data, uint32_t count) { return m_socketImpl->Write(data, count); }

void Socket::SetTimeout(uint32_t milliseconds) { m_socketImpl->SetTimeout(milliseconds); }
}  // namespace platform */
