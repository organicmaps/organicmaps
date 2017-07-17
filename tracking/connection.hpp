#pragma once

#include "coding/traffic.hpp"

#include "std/cstdint.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#include "boost/circular_buffer.hpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace platform
{
class Socket;
}

namespace tracking
{
using DataPoint = coding::TrafficGPSEncoder::DataPoint;

class Connection final
{
public:
  Connection(unique_ptr<platform::Socket> socket, string const & host, uint16_t port,
             bool isHistorical);
  bool Reconnect();
  void Shutdown();
  bool Send(boost::circular_buffer<DataPoint> const & points);

private:
  unique_ptr<platform::Socket> m_socket;
  string const m_host;
  uint16_t const m_port;
};
}  // namespace tracking
