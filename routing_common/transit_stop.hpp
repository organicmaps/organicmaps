#pragma once

#include "routing_common/transit_types.hpp"

#include "coding/point_to_integer.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace routing
{
namespace transit
{
class Stop final
{
  friend std::string DebugPrint(Stop const & stop);

public:
  Stop() = default;
  Stop(StopId id, FeatureId featureId, std::vector<LineId> const & lineIds, m2::PointD const & point);
  bool IsEqualForTesting(Stop const & stop) const;

  template <class TSink>
  void Serialize(TSink & sink) const
  {
    WriteToSink(sink, m_id);
    WriteToSink(sink, m_featureId);
    WriteToSink(sink, static_cast<uint32_t>(m_lineIds.size()));
    for (LineId lineId : m_lineIds)
      WriteToSink(sink, lineId);

    m2::PointU const pointU = PointD2PointU(m_point, POINT_COORD_BITS);
    WriteToSink(sink, pointU.x);
    WriteToSink(sink, pointU.y);
  }

  template <class TSource>
  void Deserialize(TSource & src)
  {
    m_id = ReadPrimitiveFromSource<StopId>(src);
    m_featureId = ReadPrimitiveFromSource<FeatureId>(src);
    uint32_t const lineIdsSize = ReadPrimitiveFromSource<uint32_t>(src);
    m_lineIds.resize(lineIdsSize);
    for (uint32_t i = 0; i < lineIdsSize; ++i)
      m_lineIds[i] = ReadPrimitiveFromSource<LineId>(src);
    m2::PointU pointU;
    pointU.x = ReadPrimitiveFromSource<uint32_t>(src);
    pointU.y = ReadPrimitiveFromSource<uint32_t>(src);
    m_point = PointU2PointD(pointU, POINT_COORD_BITS);
  }

private:
  StopId m_id = kStopIdInvalid;
  FeatureId m_featureId = kFeatureIdInvalid;
  std::vector<LineId> m_lineIds;
  m2::PointD m_point;
  // @TODO(bykoianko) It's necessary to add field m_titleAnchors here and implement serialization
  // and deserialization.
};

std::string DebugPrint(Stop const & stop);

// @TODO(bykoianko) Method bool DeserializeStops(Stops const & stops) should be implemented here.
// This method will parse table "stops" at TRANSIT_FILE_TAG mwm section and fills its parameter.

// @TODO(bykoianko) Data structures and methods for other transit data should be implemented in
// separate units.
}  // namespace transit
}  // namespace routing
