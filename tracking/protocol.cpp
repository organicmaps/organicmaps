#include "tracking/protocol.hpp"

#include "coding/endianness.hpp"
#include "coding/writer.hpp"

#include "base/assert.hpp"

#include "std/cstdint.hpp"
#include "std/sstream.hpp"
#include "std/utility.hpp"

namespace
{
template <typename Container>
vector<uint8_t> CreateDataPacketImpl(Container const & points)
{
  vector<uint8_t> buffer;
  MemWriter<decltype(buffer)> writer(buffer);
  tracking::Protocol::Encoder::SerializeDataPoints(tracking::Protocol::Encoder::kLatestVersion,
                                                   writer, points);

  auto packet = tracking::Protocol::CreateHeader(tracking::Protocol::PacketType::CurrentData,
                                                 static_cast<uint32_t>(buffer.size()));
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
vector<uint8_t> Protocol::CreateDataPacket(DataElementsCirc const & points)
{
  return CreateDataPacketImpl(points);
}

//  static
vector<uint8_t> Protocol::CreateDataPacket(DataElementsVec const & points)
{
  return CreateDataPacketImpl(points);
}

//  static
pair<Protocol::PacketType, size_t> Protocol::DecodeHeader(vector<uint8_t> const & data)
{
  ASSERT_GREATER_OR_EQUAL(data.size(), sizeof(uint32_t /* header */), ());

  uint32_t size = (*reinterpret_cast<uint32_t const *>(data.data())) & 0xFFFFFF00;
  if (!IsBigEndian())
    size = ReverseByteOrder(size);

  return make_pair(PacketType(static_cast<uint8_t>(data[0])), size);
}

//  static
string Protocol::DecodeAuthPacket(Protocol::PacketType type, vector<uint8_t> const & data)
{
  switch (type)
  {
  case Protocol::PacketType::AuthV0:
    return string(begin(data), end(data));
  case Protocol::PacketType::DataV0: break;
  }
  return string();
}

//  static
Protocol::DataElementsVec Protocol::DecodeDataPacket(PacketType type, vector<uint8_t> const & data)
{
  DataElementsVec points;
  MemReader memReader(data.data(), data.size());
  ReaderSource<MemReader> src(memReader);
  switch (type)
  {
  case Protocol::PacketType::DataV0:
    Encoder::DeserializeDataPoints(Encoder::kLatestVersion, src, points);
    break;
  case Protocol::PacketType::AuthV0: break;
  }
  return points;
}

//  static
void Protocol::InitHeader(vector<uint8_t> & packet, PacketType type, uint32_t payloadSize)
{
  packet.resize(sizeof(uint32_t));
  uint32_t & size = *reinterpret_cast<uint32_t *>(packet.data());
  size = payloadSize;

  ASSERT_LESS(size, 0x00FFFFFF, ());

  if (!IsBigEndian())
    size = ReverseByteOrder(size);

  packet[0] = static_cast<uint8_t>(type);
}

string DebugPrint(Protocol::PacketType type)
{
  switch (type)
  {
  case Protocol::PacketType::AuthV0: return "AuthV0";
  case Protocol::PacketType::DataV0: return "DataV0";
  }
  stringstream ss;
  ss << "Unknown(" << static_cast<uint32_t>(type) << ")";
  return ss.str();
}
}  // namespace tracking
