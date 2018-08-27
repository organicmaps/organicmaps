#pragma once

#include "coding/compressed_bit_vector.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace routing
{
struct CityRoadsHeader
{
  template <class Sink>
  void Serialize(Sink & sink) const
  {
    WriteToSink(sink, m_version);
    WriteToSink(sink, m_reserved);
  }

  template <class Source>
  void Deserialize(Source & src)
  {
    m_version = ReadPrimitiveFromSource<uint16_t>(src);
    m_reserved = ReadPrimitiveFromSource<uint16_t>(src);
  }

  uint16_t m_version = 0;
  uint16_t m_reserved = 0;
};

static_assert(sizeof(CityRoadsHeader) == 4, "Wrong header size of city_roads section.");

class CityRoadsSerializer
{
public:
  template <class Sink>
  static void Serialize(coding::CompressedBitVector const & cityRoadFeatureIds, Sink & sink)
  {
    cityRoadFeatureIds.Serialize(sink);
  }

  template <class Source>
  static std::unique_ptr<coding::CompressedBitVector> Deserialize(Source & src)
  {
    return coding::CompressedBitVectorBuilder::DeserializeFromSource(src);
  }
};
}  // namespace routing
