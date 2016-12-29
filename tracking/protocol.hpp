#pragma once

#include "coding/traffic.hpp"

#include "std/string.hpp"
#include "std/vector.hpp"

#include "boost/circular_buffer.hpp"

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

    CurrentAuth = AuthV0,
    CurrentData = DataV0
  };

  static vector<uint8_t> CreateHeader(PacketType type, uint32_t payloadSize);
  static vector<uint8_t> CreateAuthPacket(string const & clientId);
  static vector<uint8_t> CreateDataPacket(DataElementsCirc const & points);
  static vector<uint8_t> CreateDataPacket(DataElementsVec const & points);

  static std::pair<PacketType, size_t> DecodeHeader(vector<uint8_t> const & data);
  static string DecodeAuthPacket(PacketType type, vector<uint8_t> const & data);
  static DataElementsVec DecodeDataPacket(PacketType type, vector<uint8_t> const & data);

private:
  static void InitHeader(vector<uint8_t> & packet, PacketType type, uint32_t payloadSize);
};

string DebugPrint(Protocol::PacketType type);
}  // namespace tracking
