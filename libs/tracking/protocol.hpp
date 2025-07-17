#pragma once

#include "coding/traffic.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-copy"
#endif
#include <boost/circular_buffer.hpp>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

namespace tracking
{
class Protocol
{
public:
  using Encoder = coding::TrafficGPSEncoder;
  using DataElementsCirc = boost::circular_buffer<Encoder::DataPoint>;
  using DataElementsVec = std::vector<Encoder::DataPoint>;

  static uint8_t const kOk[4];
  static uint8_t const kFail[4];

  enum class PacketType
  {
    Error = 0x0,
    AuthV0 = 0x81,
    DataV0 = 0x82,
    DataV1 = 0x92,

    CurrentAuth = AuthV0,
    CurrentData = DataV1
  };

  static std::vector<uint8_t> CreateHeader(PacketType type, uint32_t payloadSize);
  static std::vector<uint8_t> CreateAuthPacket(std::string const & clientId);
  static std::vector<uint8_t> CreateDataPacket(DataElementsCirc const & points, PacketType type);
  static std::vector<uint8_t> CreateDataPacket(DataElementsVec const & points, PacketType type);

  static std::pair<PacketType, size_t> DecodeHeader(std::vector<uint8_t> const & data);
  static std::string DecodeAuthPacket(PacketType type, std::vector<uint8_t> const & data);
  static DataElementsVec DecodeDataPacket(PacketType type, std::vector<uint8_t> const & data);

private:
  static void InitHeader(std::vector<uint8_t> & packet, PacketType type, uint32_t payloadSize);
};

std::string DebugPrint(Protocol::PacketType type);
}  // namespace tracking
