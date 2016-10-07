#pragma once

#include "boost/circular_buffer.hpp"

#include "coding/traffic.hpp"

#include "std/vector.hpp"

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
  static const char kHost[];
  static uint16_t kPort;

  Connection(unique_ptr<platform::Socket> socket, string const & host, uint16_t port,
             bool isHistorical);
  bool Reconnect();
  bool Send(boost::circular_buffer<DataPoint> const & points);

private:
  unique_ptr<platform::Socket> m_socket;
  string const m_host;
  uint16_t const m_port;
  vector<uint8_t> m_buffer;
};
}  // namespace tracking
