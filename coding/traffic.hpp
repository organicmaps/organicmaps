#pragma once

#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "geometry/latlon.hpp"

#include "std/limits.hpp"
#include "std/vector.hpp"

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
    DataPoint() = default;

    DataPoint(uint64_t timestamp, ms::LatLon latLon) : m_timestamp(timestamp), m_latLon(latLon) {}
    // Uint64 should be enough for all our use cases.
    // It is expected that |m_timestamp| stores time since epoch in seconds.
    uint64_t m_timestamp = 0;
    ms::LatLon m_latLon = ms::LatLon::Zero();
  };

  // Serializes |points| to |writer| by storing delta-encoded points.
  // Returns the number of bytes written.
  // Version 0:
  //   Coordinates are truncated and stored as integers. All integers
  //   are written as varints.
  template <typename Writer, typename Collection>
  static size_t SerializeDataPoints(uint32_t version, Writer & writer,
                                    Collection const & points)
  {
    ASSERT_LESS_OR_EQUAL(version, kLatestVersion, ());

    auto const startPos = writer.Pos();

    if (!points.empty())
    {
      uint64_t const firstTimestamp = points[0].m_timestamp;
      uint32_t const firstLat =
          DoubleToUint32(points[0].m_latLon.lat, ms::LatLon::kMinLat, ms::LatLon::kMaxLat);
      uint32_t const firstLon =
          DoubleToUint32(points[0].m_latLon.lon, ms::LatLon::kMinLon, ms::LatLon::kMaxLon);
      WriteVarUint(writer, firstTimestamp);
      WriteVarUint(writer, firstLat);
      WriteVarUint(writer, firstLon);
    }

    for (size_t i = 1; i < points.size(); ++i)
    {
      ASSERT_LESS_OR_EQUAL(points[i - 1].m_timestamp, points[i].m_timestamp, ());

      uint64_t const deltaTimestamp = points[i].m_timestamp - points[i - 1].m_timestamp;
      uint32_t deltaLat = DoubleToUint32(points[i].m_latLon.lat - points[i - 1].m_latLon.lat,
                                         kMinDeltaLat, kMaxDeltaLat);
      uint32_t deltaLon = DoubleToUint32(points[i].m_latLon.lon - points[i - 1].m_latLon.lon,
                                         kMinDeltaLon, kMaxDeltaLon);

      WriteVarUint(writer, deltaTimestamp);
      WriteVarUint(writer, deltaLat);
      WriteVarUint(writer, deltaLon);
    }

    ASSERT_LESS_OR_EQUAL(writer.Pos() - startPos, numeric_limits<size_t>::max(),
                         ("Too much data."));
    return static_cast<size_t>(writer.Pos() - startPos);
  }

  // Deserializes the points from |source| and appends them to |result|.
  template <typename Source, typename Collection>
  static void DeserializeDataPoints(uint32_t version, Source & src, Collection & result)
  {
    ASSERT_LESS_OR_EQUAL(version, kLatestVersion, ());

    bool first = true;
    uint64_t lastTimestamp = 0;
    double lastLat = 0.0;
    double lastLon = 0.0;

    while (src.Size() > 0)
    {
      if (first)
      {
        lastTimestamp = ReadVarUint<uint64_t>(src);
        lastLat =
            Uint32ToDouble(ReadVarUint<uint32_t>(src), ms::LatLon::kMinLat, ms::LatLon::kMaxLat);
        lastLon =
            Uint32ToDouble(ReadVarUint<uint32_t>(src), ms::LatLon::kMinLon, ms::LatLon::kMaxLon);
        result.emplace_back(lastTimestamp, ms::LatLon(lastLat, lastLon));
        first = false;
      }
      else
      {
        lastTimestamp += ReadVarUint<uint64_t>(src);
        lastLat += Uint32ToDouble(ReadVarUint<uint32_t>(src), kMinDeltaLat, kMaxDeltaLat);
        lastLon += Uint32ToDouble(ReadVarUint<uint32_t>(src), kMinDeltaLon, kMaxDeltaLon);
        result.emplace_back(lastTimestamp, ms::LatLon(lastLat, lastLon));
      }
    }
  }

private:
  static uint32_t DoubleToUint32(double x, double min, double max);

  static double Uint32ToDouble(uint32_t x, double min, double max);
};
}  // namespace coding
