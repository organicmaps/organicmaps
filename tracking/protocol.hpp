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
  using DataElements = boost::circular_buffer<Encoder::DataPoint>;

  static uint8_t const kOk[4];
  static uint8_t const kFail[4];

  enum class PacketType
  {
    AuthV0 = 0x81,
    DataV0 = 0x82,

    CurrentAuth = AuthV0,
    CurrentData = DataV0
  };

  static vector<uint8_t> CreateAuthPacket(string const & clientId);
  static vector<uint8_t> CreateDataPacket(DataElements const & points);

private:
  static void InitHeader(vector<uint8_t> & packet, PacketType type, uint32_t payloadSize);
};

string DebugPrint(Protocol::PacketType type);
}  // namespace tracking
