#include "tracking/protocol.hpp"

#include "coding/endianness.hpp"

#include "base/assert.hpp"

#include "std/cstdint.hpp"

namespace tracking
{
uint8_t const Protocol::kOk[4] = {'O', 'K', '\n', '\n'};
uint8_t const Protocol::kFail[4] = {'F', 'A', 'I', 'L'};

static_assert(sizeof(Protocol::kFail) >= sizeof(Protocol::kOk), "");

vector<uint8_t> Protocol::CreateAuthPacket(string const & clientId)
{
  vector<uint8_t> packet;

  InitHeader(packet, PacketType::CurrentAuth, static_cast<uint32_t>(clientId.size()));
  packet.insert(packet.end(), begin(clientId), end(clientId));

  return packet;
}

vector<uint8_t> Protocol::CreateDataPacket(DataElements const & points)
{
  vector<uint8_t> buffer;
  MemWriter<decltype(buffer)> writer(buffer);
  Encoder::SerializeDataPoints(Encoder::kLatestVersion, writer, points);

  vector<uint8_t> packet;
  InitHeader(packet, PacketType::CurrentData, static_cast<uint32_t>(buffer.size()));
  packet.insert(packet.end(), begin(buffer), end(buffer));

  return packet;
}

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
    default: return "Unknown";
  }
}
}  // namespace tracking
