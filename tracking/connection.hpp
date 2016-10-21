#pragma once

#include "coding/traffic.hpp"

#include "std/cstdint.hpp"
#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

#include "boost/circular_buffer.hpp"

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
  bool Send(boost::circular_buffer<DataPoint> const & points);

private:
  unique_ptr<platform::Socket> m_socket;
  string const m_host;
  uint16_t const m_port;
  bool const m_isHistorical;
};
}  // namespace tracking
