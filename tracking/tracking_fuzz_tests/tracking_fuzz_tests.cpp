#include "tracking/protocol.hpp"

#include "coding/traffic.hpp"

#include "geometry/latlon.hpp"

#include "base/logging.hpp"

#include <cstdint>
#include <iostream>
#include <iterator>
#include <vector>

using namespace coding;
using namespace tracking;

namespace
{
template <typename T>
T PopType(std::vector<uint8_t> & data)
{
  T t{};
  if (data.empty())
    return t;

  if (data.size() < sizeof(T))
  {
    data.clear();
    return t;
  }

  t = *reinterpret_cast<T *>(data.data());
  data.erase(data.begin(), std::next(data.begin(), sizeof(T)));
  return t;
}

TrafficGPSEncoder::DataPoint PopDataPoint(std::vector<uint8_t> & data)
{
  auto const timestamp = PopType<uint64_t>(data);
  auto const lat = PopType<double>(data);
  auto const lon = PopType<double>(data);
  auto const traffic = PopType<uint8_t>(data);
  return TrafficGPSEncoder::DataPoint(timestamp, ms::LatLon(lat, lon), traffic);
}
}  // namespace

extern "C" int LLVMFuzzerTestOneInput(uint8_t const * data, size_t size)
{
  base::ScopedLogLevelChanger scopedLogLevelChanger(base::LCRITICAL);

  std::vector<uint8_t> const dataVec(data, data + size);

  auto dataVecToConv = dataVec;
  Protocol::DataElementsVec dataElementsVec;
  while (!dataVecToConv.empty())
    dataElementsVec.push_back(PopDataPoint(dataVecToConv));
  Protocol::DataElementsCirc dataElementsCirc(dataElementsVec.cbegin(), dataElementsVec.cend());

  Protocol::DecodeHeader(dataVec);
  for (auto const type : {Protocol::PacketType::Error, Protocol::PacketType::AuthV0, Protocol::PacketType::DataV0,
                          Protocol::PacketType::DataV1})
  {
    Protocol::CreateDataPacket(dataElementsVec, type);
    Protocol::CreateDataPacket(dataElementsCirc, type);
    Protocol::DecodeAuthPacket(type, dataVec);
    Protocol::DecodeDataPacket(type, dataVec);
  }
  return 0;
}
