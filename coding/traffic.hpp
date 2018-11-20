#pragma once

#include "coding/point_coding.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "geometry/latlon.hpp"

#include "base/checked_cast.hpp"

#include <limits>
#include <string>

namespace coding
{
class TrafficGPSEncoder
{
public:
  static uint32_t const kLatestVersion;
  static uint32_t const kCoordBits;
  static double const kMinDeltaLat;
  static double const kMaxDeltaLat;
  static double const kMinDeltaLon;
  static double const kMaxDeltaLon;

  struct DataPoint
  {
    // TODO(@m): document the version format
    DataPoint() = default;

    DataPoint(uint64_t timestamp, ms::LatLon latLon, uint8_t traffic)
      : m_timestamp(timestamp), m_latLon(latLon), m_traffic(traffic)
    {
    }
    // Uint64 should be enough for all our use cases.
    // It is expected that |m_timestamp| stores time since epoch in seconds.
    uint64_t m_timestamp = 0;
    ms::LatLon m_latLon = ms::LatLon::Zero();
    // A pod type instead of the traffic::SpeedGroup enum
    // so as not to introduce a cyclic dependency.
    // This field was added in Version 1 (and was the only addition).
    uint8_t m_traffic = 0;

    bool operator==(DataPoint const & p) const
    {
      return m_timestamp == p.m_timestamp && m_latLon == p.m_latLon;
    }
  };

  // Serializes |points| to |writer| by storing delta-encoded points.
  // Returns the number of bytes written.
  // Version 0:
  //   Coordinates are truncated and stored as integers. All integers
  //   are written as varints.
  template <typename Writer, typename Collection>
  static size_t SerializeDataPoints(uint32_t version, Writer & writer, Collection const & points)
  {
    switch (version)
    {
    case 0: return SerializeDataPointsV0(writer, points);
    case 1: return SerializeDataPointsV1(writer, points);

    default: ASSERT(false, ("Unexpected serializer version:", version)); break;
    }
    return 0;
  }

  // Deserializes the points from |source| and appends them to |result|.
  template <typename Source, typename Collection>
  static void DeserializeDataPoints(uint32_t version, Source & src, Collection & result)
  {
    switch (version)
    {
    case 0: return DeserializeDataPointsV0(src, result);
    case 1: return DeserializeDataPointsV1(src, result);

    default: ASSERT(false, ("Unexpected serializer version:", version)); break;
    }
  }

private:
  template <typename Writer, typename Collection>
  static size_t SerializeDataPointsV0(Writer & writer, Collection const & points)
  {
    auto const startPos = writer.Pos();

    if (!points.empty())
    {
      uint64_t const firstTimestamp = points[0].m_timestamp;
      uint32_t const firstLat = DoubleToUint32(points[0].m_latLon.lat, ms::LatLon::kMinLat,
                                               ms::LatLon::kMaxLat, kCoordBits);
      uint32_t const firstLon = DoubleToUint32(points[0].m_latLon.lon, ms::LatLon::kMinLon,
                                               ms::LatLon::kMaxLon, kCoordBits);
      WriteVarUint(writer, firstTimestamp);
      WriteVarUint(writer, firstLat);
      WriteVarUint(writer, firstLon);
    }

    for (size_t i = 1; i < points.size(); ++i)
    {
      ASSERT_LESS_OR_EQUAL(points[i - 1].m_timestamp, points[i].m_timestamp, ());

      uint64_t const deltaTimestamp = points[i].m_timestamp - points[i - 1].m_timestamp;
      uint32_t deltaLat = DoubleToUint32(points[i].m_latLon.lat - points[i - 1].m_latLon.lat,
                                         kMinDeltaLat, kMaxDeltaLat, kCoordBits);
      uint32_t deltaLon = DoubleToUint32(points[i].m_latLon.lon - points[i - 1].m_latLon.lon,
                                         kMinDeltaLon, kMaxDeltaLon, kCoordBits);

      WriteVarUint(writer, deltaTimestamp);
      WriteVarUint(writer, deltaLat);
      WriteVarUint(writer, deltaLon);
    }

    ASSERT_LESS_OR_EQUAL(writer.Pos() - startPos, std::numeric_limits<size_t>::max(),
                         ("Too much data."));
    return static_cast<size_t>(writer.Pos() - startPos);
  }

  template <typename Writer, typename Collection>
  static size_t SerializeDataPointsV1(Writer & writer, Collection const & points)
  {
    auto const startPos = writer.Pos();

    if (!points.empty())
    {
      uint64_t const firstTimestamp = points[0].m_timestamp;
      uint32_t const firstLat = DoubleToUint32(points[0].m_latLon.lat, ms::LatLon::kMinLat,
                                               ms::LatLon::kMaxLat, kCoordBits);
      uint32_t const firstLon = DoubleToUint32(points[0].m_latLon.lon, ms::LatLon::kMinLon,
                                               ms::LatLon::kMaxLon, kCoordBits);
      uint32_t const traffic = points[0].m_traffic;
      WriteVarUint(writer, firstTimestamp);
      WriteVarUint(writer, firstLat);
      WriteVarUint(writer, firstLon);
      WriteVarUint(writer, traffic);
    }

    for (size_t i = 1; i < points.size(); ++i)
    {
      ASSERT_LESS_OR_EQUAL(points[i - 1].m_timestamp, points[i].m_timestamp, ());

      uint64_t const deltaTimestamp = points[i].m_timestamp - points[i - 1].m_timestamp;
      uint32_t deltaLat = DoubleToUint32(points[i].m_latLon.lat - points[i - 1].m_latLon.lat,
                                         kMinDeltaLat, kMaxDeltaLat, kCoordBits);
      uint32_t deltaLon = DoubleToUint32(points[i].m_latLon.lon - points[i - 1].m_latLon.lon,
                                         kMinDeltaLon, kMaxDeltaLon, kCoordBits);

      uint32_t const traffic = points[i - 1].m_traffic;
      WriteVarUint(writer, deltaTimestamp);
      WriteVarUint(writer, deltaLat);
      WriteVarUint(writer, deltaLon);
      WriteVarUint(writer, traffic);
    }

    ASSERT_LESS_OR_EQUAL(writer.Pos() - startPos, std::numeric_limits<size_t>::max(),
                         ("Too much data."));
    return static_cast<size_t>(writer.Pos() - startPos);
  }

  template <typename Source, typename Collection>
  static void DeserializeDataPointsV0(Source & src, Collection & result)
  {
    bool first = true;
    uint64_t lastTimestamp = 0;
    double lastLat = 0.0;
    double lastLon = 0.0;
    uint8_t traffic = 0;

    while (src.Size() > 0)
    {
      if (first)
      {
        lastTimestamp = ReadVarUint<uint64_t>(src);
        lastLat = Uint32ToDouble(ReadVarUint<uint32_t>(src), ms::LatLon::kMinLat,
                                 ms::LatLon::kMaxLat, kCoordBits);
        lastLon = Uint32ToDouble(ReadVarUint<uint32_t>(src), ms::LatLon::kMinLon,
                                 ms::LatLon::kMaxLon, kCoordBits);
        result.emplace_back(lastTimestamp, ms::LatLon(lastLat, lastLon), traffic);
        first = false;
      }
      else
      {
        lastTimestamp += ReadVarUint<uint64_t>(src);
        lastLat +=
            Uint32ToDouble(ReadVarUint<uint32_t>(src), kMinDeltaLat, kMaxDeltaLat, kCoordBits);
        lastLon +=
            Uint32ToDouble(ReadVarUint<uint32_t>(src), kMinDeltaLon, kMaxDeltaLon, kCoordBits);
        result.emplace_back(lastTimestamp, ms::LatLon(lastLat, lastLon), traffic);
      }
    }
  }

  template <typename Source, typename Collection>
  static void DeserializeDataPointsV1(Source & src, Collection & result)
  {
    bool first = true;
    uint64_t lastTimestamp = 0;
    double lastLat = 0.0;
    double lastLon = 0.0;
    uint8_t traffic = 0;

    while (src.Size() > 0)
    {
      if (first)
      {
        lastTimestamp = ReadVarUint<uint64_t>(src);
        lastLat = Uint32ToDouble(ReadVarUint<uint32_t>(src), ms::LatLon::kMinLat,
                                 ms::LatLon::kMaxLat, kCoordBits);
        lastLon = Uint32ToDouble(ReadVarUint<uint32_t>(src), ms::LatLon::kMinLon,
                                 ms::LatLon::kMaxLon, kCoordBits);
        traffic = base::asserted_cast<uint8_t>(ReadVarUint<uint32_t>(src));
        result.emplace_back(lastTimestamp, ms::LatLon(lastLat, lastLon), traffic);
        first = false;
      }
      else
      {
        lastTimestamp += ReadVarUint<uint64_t>(src);
        lastLat +=
            Uint32ToDouble(ReadVarUint<uint32_t>(src), kMinDeltaLat, kMaxDeltaLat, kCoordBits);
        lastLon +=
            Uint32ToDouble(ReadVarUint<uint32_t>(src), kMinDeltaLon, kMaxDeltaLon, kCoordBits);
        traffic = base::asserted_cast<uint8_t>(ReadVarUint<uint32_t>(src));
        result.emplace_back(lastTimestamp, ms::LatLon(lastLat, lastLon), traffic);
      }
    }
  }
};
}  // namespace coding
