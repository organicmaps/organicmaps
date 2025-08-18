#pragma once

#include "coding/traffic.hpp"

#include <cstdint>
#include <memory>
#include <string>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-copy"
#endif
#include <boost/circular_buffer.hpp>
#ifdef __clang__
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
  Connection(std::unique_ptr<platform::Socket> socket, std::string const & host, uint16_t port, bool isHistorical);
  bool Reconnect();
  void Shutdown();
  bool Send(boost::circular_buffer<DataPoint> const & points);

private:
  std::unique_ptr<platform::Socket> m_socket;
  std::string const m_host;
  uint16_t const m_port;
};
}  // namespace tracking
