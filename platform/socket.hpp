#pragma once

#include "std/string.hpp"
#include "std/target_os.hpp"

#if defined(OMIM_OS_IPHONE) || defined(OMIM_OS_MAC)
@class SocketImpl;
#else
class SocketImpl;
#endif

namespace platform
{
class Socket
{
public:
  Socket();
  ~Socket();

  bool Open(string const & host, uint16_t port);
  void Close();

  bool Read(uint8_t * data, uint32_t count);
  bool Write(uint8_t const * data, uint32_t count);

  void SetTimeout(uint32_t milliseconds);

private:
  SocketImpl * m_socketImpl = nullptr;
};
}  // namespace platform
