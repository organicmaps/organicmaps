#include "tracking/protocol.hpp"

#include "coding/endianness.hpp"
#include "coding/reader.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include <sstream>

using namespace std;

namespace
{
template <typename Container>
vector<uint8_t> CreateDataPacketImpl(Container const & points, tracking::Protocol::PacketType const type)
{
  vector<uint8_t> buffer;
  MemWriter<decltype(buffer)> writer(buffer);

  uint32_t version = tracking::Protocol::Encoder::kLatestVersion;
  switch (type)
  {
  case tracking::Protocol::PacketType::DataV0: version = 0; break;
  case tracking::Protocol::PacketType::DataV1: version = 1; break;
  case tracking::Protocol::PacketType::Error:
  case tracking::Protocol::PacketType::AuthV0:
    LOG(LERROR, ("Can't create a non-DATA packet as a DATA packet. PacketType =", type));
    return {};
  }

  tracking::Protocol::Encoder::SerializeDataPoints(version, writer, points);

  auto packet = tracking::Protocol::CreateHeader(type, static_cast<uint32_t>(buffer.size()));
  packet.insert(packet.end(), begin(buffer), end(buffer));

  return packet;
}
}  // namespace

namespace tracking
{
uint8_t const Protocol::kOk[4] = {'O', 'K', '\n', '\n'};
uint8_t const Protocol::kFail[4] = {'F', 'A', 'I', 'L'};

static_assert(sizeof(Protocol::kFail) >= sizeof(Protocol::kOk), "");

//  static
vector<uint8_t> Protocol::CreateHeader(PacketType type, uint32_t payloadSize)
{
  vector<uint8_t> header;
  InitHeader(header, type, payloadSize);
  return header;
}

//  static
vector<uint8_t> Protocol::CreateAuthPacket(string const & clientId)
{
  vector<uint8_t> packet;

  InitHeader(packet, PacketType::CurrentAuth, static_cast<uint32_t>(clientId.size()));
  packet.insert(packet.end(), begin(clientId), end(clientId));

  return packet;
}

//  static
vector<uint8_t> Protocol::CreateDataPacket(DataElementsCirc const & points, PacketType type)
{
  return CreateDataPacketImpl(points, type);
}

//  static
vector<uint8_t> Protocol::CreateDataPacket(DataElementsVec const & points, PacketType type)
{
  return CreateDataPacketImpl(points, type);
}

//  static
pair<Protocol::PacketType, size_t> Protocol::DecodeHeader(vector<uint8_t> const & data)
{
  if (data.size() < sizeof(uint32_t /* header */))
  {
    LOG(LWARNING, ("Header size is too small", data.size(), sizeof(uint32_t /* header */)));
    return make_pair(PacketType::Error, data.size());
  }

  uint32_t size = (*reinterpret_cast<uint32_t const *>(data.data())) & 0xFFFFFF00;
  if (!IsBigEndianMacroBased())
    size = ReverseByteOrder(size);
  return make_pair(PacketType(static_cast<uint8_t>(data[0])), size);
}

//  static
string Protocol::DecodeAuthPacket(Protocol::PacketType type, vector<uint8_t> const & data)
{
  switch (type)
  {
  case Protocol::PacketType::AuthV0: return string(begin(data), end(data));
  case Protocol::PacketType::Error:
  case Protocol::PacketType::DataV0:
  case Protocol::PacketType::DataV1: LOG(LERROR, ("Error decoding AUTH packet. PacketType =", type)); break;
  }
  return string();
}

//  static
Protocol::DataElementsVec Protocol::DecodeDataPacket(PacketType type, vector<uint8_t> const & data)
{
  DataElementsVec points;
  MemReaderWithExceptions memReader(data.data(), data.size());
  ReaderSource<MemReaderWithExceptions> src(memReader);
  try
  {
    switch (type)
    {
    case Protocol::PacketType::DataV0: Encoder::DeserializeDataPoints(0 /* version */, src, points); break;
    case Protocol::PacketType::DataV1: Encoder::DeserializeDataPoints(1 /* version */, src, points); break;
    case Protocol::PacketType::Error:
    case Protocol::PacketType::AuthV0: LOG(LERROR, ("Error decoding DATA packet. PacketType =", type)); return {};
    }
    return points;
  }
  catch (Reader::SizeException const & ex)
  {
    LOG(LWARNING, ("Wrong packet. SizeException. Msg:", ex.Msg(), ". What:", ex.what()));
    return {};
  }
}

//  static
void Protocol::InitHeader(vector<uint8_t> & packet, PacketType type, uint32_t payloadSize)
{
  packet.resize(sizeof(uint32_t));
  uint32_t & size = *reinterpret_cast<uint32_t *>(packet.data());
  size = payloadSize;

  ASSERT_LESS(size, 0x00FFFFFF, ());

  if (!IsBigEndianMacroBased())
    size = ReverseByteOrder(size);

  packet[0] = static_cast<uint8_t>(type);
}

string DebugPrint(Protocol::PacketType type)
{
  switch (type)
  {
  case Protocol::PacketType::Error: return "Error";
  case Protocol::PacketType::AuthV0: return "AuthV0";
  case Protocol::PacketType::DataV0: return "DataV0";
  case Protocol::PacketType::DataV1: return "DataV1";
  }
  stringstream ss;
  ss << "Unknown(" << static_cast<uint32_t>(type) << ")";
  return ss.str();
}
}  // namespace tracking
