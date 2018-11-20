#pragma once

#include "routing/route.hpp"
#include "routing/routing_session.hpp"
#include "routing/segment.hpp"
#include "routing/speed_camera.hpp"

#include "coding/file_container.hpp"
#include "coding/file_writer.hpp"
#include "coding/point_coding.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "geometry/point2d.hpp"

#include "base/assert.hpp"

#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace routing
{
static uint8_t constexpr kMaxCameraSpeedKmpH = std::numeric_limits<uint8_t>::max();

/// \brief |m_featureId| and |m_segmentId| identify the place where the camera is located.
/// |m_coef| is a factor [0, 1] where the camera is placed at the segment. |m_coef| == 0
/// means the camera is placed at the beginning of the segment.
struct SpeedCameraMwmPosition
{
  SpeedCameraMwmPosition() = default;
  SpeedCameraMwmPosition(uint32_t fId, uint32_t sId, double k)
    : m_featureId(fId), m_segmentId(sId), m_coef(k) {}

  uint32_t m_featureId = 0;
  uint32_t m_segmentId = 0;
  double m_coef = 0.0;
};

// Don't touch the order of enum (this is will be part of mwm).
// TODO (@gmoryes) change comment after adding section in mwm.
enum class SpeedCameraDirection
{
  Unknown = 0,
  Forward = 1,
  Backward = 2,
  Both = 3
};

struct SpeedCameraMetadata
{
  SpeedCameraMetadata() = default;
  SpeedCameraMetadata(m2::PointD const & center, uint8_t maxSpeed,
                      std::vector<routing::SpeedCameraMwmPosition> && ways)
    : m_center(center), m_maxSpeedKmPH(maxSpeed), m_ways(std::move(ways)) {}

  m2::PointD m_center;
  uint8_t m_maxSpeedKmPH = 0;
  std::vector<routing::SpeedCameraMwmPosition> m_ways;
  SpeedCameraDirection m_direction = SpeedCameraDirection::Unknown;
};

class SpeedCameraMwmHeader
{
public:
  static uint32_t constexpr kLatestVersion = 0;

  void SetVersion(uint32_t version) { m_version = version; }
  void SetAmount(uint32_t amount) { m_amount = amount; }
  uint32_t GetAmount() const { return m_amount; }

  template <typename T>
  void Serialize(T & sink) const
  {
    WriteToSink(sink, m_version);
    WriteToSink(sink, m_amount);
  }

  template <typename T>
  void Deserialize(T & sink)
  {
    ReadPrimitiveFromSource(sink, m_version);
    ReadPrimitiveFromSource(sink, m_amount);
  }

  bool IsValid() const { return m_version <= kLatestVersion; }

private:
  uint32_t m_version = 0;
  uint32_t m_amount = 0;
};

static_assert(sizeof(SpeedCameraMwmHeader) == 8, "Strange size of speed camera section header");

struct SegmentCoord
{
  SegmentCoord() = default;
  SegmentCoord(uint32_t fId, uint32_t sId) : m_featureId(fId), m_segmentId(sId) {}

  bool operator<(SegmentCoord const & rhs) const
  {
    if (m_featureId != rhs.m_featureId)
      return m_featureId < rhs.m_featureId;

    return m_segmentId < rhs.m_segmentId;
  }

  uint32_t m_featureId = 0;
  uint32_t m_segmentId = 0;
};

void SerializeSpeedCamera(FileWriter & writer, SpeedCameraMetadata const & data,
                          uint32_t & prevFeatureId);

template <typename Reader>
std::pair<SegmentCoord, RouteSegment::SpeedCamera> DeserializeSpeedCamera(
  ReaderSource<Reader> & src, uint32_t & prevFeatureId)
{
  auto featureId = ReadVarUint<uint32_t>(src);
  featureId += prevFeatureId;  // delta coding
  prevFeatureId = featureId;

  auto const segmentId = ReadVarUint<uint32_t>(src);

  uint32_t coefInt = 0;
  ReadPrimitiveFromSource(src, coefInt);
  double const coef = Uint32ToDouble(coefInt, 0.0 /* min */, 1.0 /* max */, 32 /* bits */);

  uint8_t speed = 0;
  ReadPrimitiveFromSource(src, speed);
  CHECK_LESS(speed, kMaxCameraSpeedKmpH, ());
  if (speed == 0)
    speed = routing::SpeedCameraOnRoute::kNoSpeedInfo;

  // We don't use direction of camera, because of bad data in OSM.
  UNUSED_VALUE(ReadPrimitiveFromSource<uint8_t>(src));  // direction

  // Number of time conditions of camera.
  auto const conditionsNumber = ReadVarUint<uint32_t>(src);
  CHECK_EQUAL(conditionsNumber, 0,
              ("Number of conditions should be 0, non zero number is not implemented now"));

  return {{featureId, segmentId} /* SegmentCoord */,
          {coef, speed}          /* RouteSegment::SpeedCamera */};
}

template <typename Reader>
void DeserializeSpeedCamsFromMwm(
  ReaderSource<Reader> & src,
  std::map<SegmentCoord, std::vector<RouteSegment::SpeedCamera>> & map)
{
  SpeedCameraMwmHeader header;
  header.Deserialize(src);
  CHECK(header.IsValid(), ("Bad header of speed cam section"));
  uint32_t const amount = header.GetAmount();

  SegmentCoord segment;
  RouteSegment::SpeedCamera speedCamera;
  uint32_t prevFeatureId = 0;
  for (uint32_t i = 0; i < amount; ++i)
  {
    std::tie(segment, speedCamera) = DeserializeSpeedCamera(src, prevFeatureId);
    map[segment].emplace_back(speedCamera);
  }
}

std::string DebugPrint(SegmentCoord const & segment);
}  // namespace routing
