#include "connection.hpp"

#include "platform/socket.hpp"

namespace
{
uint32_t constexpr kSocketTimeoutMs = 10000;
}  // namespace

namespace tracking
{
// static
const char Connection::kHost[] = "gps.host";  // TODO change to real host value
uint16_t Connection::kPort = 666;             // TODO change to real port value

Connection::Connection(unique_ptr<platform::Socket> socket, string const & host, uint16_t port,
                       bool isHistorical)
  : m_socket(move(socket)), m_host(host), m_port(port)
{
  ASSERT(m_socket.get() != nullptr, ());

  m_socket->SetTimeout(kSocketTimeoutMs);
}

// TODO: implement handshake
bool Connection::Reconnect()
{
  m_socket->Close();
  return m_socket->Open(m_host, m_port);
}

// TODO: implement historical
bool Connection::Send(boost::circular_buffer<DataPoint> const & points)
{
  ASSERT(m_buffer.empty(), ());

  MemWriter<decltype(m_buffer)> writer(m_buffer);
  coding::TrafficGPSEncoder::SerializeDataPoints(coding::TrafficGPSEncoder::kLatestVersion, writer,
                                                 points);
  bool isSuccess = m_socket->Write(m_buffer.data(), m_buffer.size());
  m_buffer.clear();
  return isSuccess;
}
}  // namespace tracking
