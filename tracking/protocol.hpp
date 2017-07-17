#pragma once

#include "coding/traffic.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#include "boost/circular_buffer.hpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace tracking
{
class Protocol
{
public:
  using Encoder = coding::TrafficGPSEncoder;
  using DataElementsCirc = boost::circular_buffer<Encoder::DataPoint>;
  using DataElementsVec = vector<Encoder::DataPoint>;

  static uint8_t const kOk[4];
  static uint8_t const kFail[4];

  enum class PacketType
  {
    AuthV0 = 0x81,
    DataV0 = 0x82,
    DataV1 = 0x92,

    CurrentAuth = AuthV0,
    CurrentData = DataV1
  };

  static vector<uint8_t> CreateHeader(PacketType type, uint32_t payloadSize);
  static vector<uint8_t> CreateAuthPacket(string const & clientId);
  static vector<uint8_t> CreateDataPacket(DataElementsCirc const & points, PacketType type);
  static vector<uint8_t> CreateDataPacket(DataElementsVec const & points, PacketType type);

  static std::pair<PacketType, size_t> DecodeHeader(vector<uint8_t> const & data);
  static string DecodeAuthPacket(PacketType type, vector<uint8_t> const & data);
  static DataElementsVec DecodeDataPacket(PacketType type, vector<uint8_t> const & data);

private:
  static void InitHeader(vector<uint8_t> & packet, PacketType type, uint32_t payloadSize);
};

string DebugPrint(Protocol::PacketType type);
}  // namespace tracking
