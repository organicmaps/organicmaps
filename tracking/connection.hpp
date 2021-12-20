#pragma once

#include "coding/traffic.hpp"

#include <cstdint>
#include <memory>
#include <string>

#include <boost/circular_buffer.hpp>

namespace platform { class Socket; }

namespace tracking
{
using DataPoint = coding::TrafficGPSEncoder::DataPoint;

class Connection final
{
public:
  Connection(std::unique_ptr<platform::Socket> socket, std::string const & host, uint16_t port,
             bool isHistorical);
  bool Reconnect();
  void Shutdown();
  bool Send(boost::circular_buffer<DataPoint> const & points);

private:
  std::unique_ptr<platform::Socket> m_socket;
  std::string const m_host;
  uint16_t const m_port;
};
}  // namespace tracking
