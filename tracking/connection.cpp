#include "connection.hpp"

#include "platform/socket.hpp"

namespace
{
uint32_t constexpr kSocketTimeoutMs = 10000;
}  // namespace

namespace tracking
{
Connection::Connection(unique_ptr<platform::Socket> socket, string const & host, uint16_t port,
                       bool isHistorical)
  : m_socket(move(socket)), m_host(host), m_port(port), m_isHistorical(isHistorical)
{
  ASSERT(m_socket.get(), ());

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
  using coding::TrafficGPSEncoder;
  TrafficGPSEncoder::SerializeDataPoints(TrafficGPSEncoder::kLatestVersion, writer, points);
  bool const isSuccess = m_socket->Write(m_buffer.data(), m_buffer.size());
  m_buffer.clear();
  return isSuccess;
}
}  // namespace tracking
