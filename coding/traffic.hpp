#pragma once

#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include "geometry/latlon.hpp"

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
    uint64_t m_timestamp = 0;
    ms::LatLon m_latLon = ms::LatLon::Zero();
  };

  // Serializes |points| to |writer| by first writing
  // a header of 4 bytes and then writing the payload
  // encoded according to |version|.
  // Version 0:
  //   * header contains the size of the payload in bytes
  //     in its top 29 (little-endian) bits and version number
  //     in its 3 low bits.
  //   * payload stores delta-encoded points. Coordinates
  //     are truncated and stored as integers. All integers
  //     are written as varints.
  template <typename Writer>
  static void SerializeDataPoints(uint32_t version, Writer & writer,
                                  vector<DataPoint> const & points)
  {
    vector<uint8_t> buf;
    MemWriter<decltype(buf)> bufWriter(buf);

    auto const startPos = bufWriter.Pos();
    uint32_t header = 0;
    // We will fill the header later.
    bufWriter.Write(&header, sizeof(header));
    auto const payloadStartPos = bufWriter.Pos();

    if (!points.empty())
    {
      uint64_t const firstTimestamp = points[0].m_timestamp;
      uint32_t const firstLat =
          DoubleToUint32(points[0].m_latLon.lat, ms::LatLon::kMinLat, ms::LatLon::kMaxLat);
      uint32_t const firstLon =
          DoubleToUint32(points[0].m_latLon.lon, ms::LatLon::kMinLon, ms::LatLon::kMaxLon);
      WriteVarUint(bufWriter, firstTimestamp);
      WriteVarUint(bufWriter, firstLat);
      WriteVarUint(bufWriter, firstLon);
    }

    for (size_t i = 1; i < points.size(); ++i)
    {
      ASSERT_LESS_OR_EQUAL(points[i - 1].m_timestamp, points[i].m_timestamp, ());

      uint64_t const deltaTimestamp = points[i].m_timestamp - points[i - 1].m_timestamp;
      uint32_t deltaLat = DoubleToUint32(points[i].m_latLon.lat - points[i - 1].m_latLon.lat,
                                         kMinDeltaLat, kMaxDeltaLat);
      uint32_t deltaLon = DoubleToUint32(points[i].m_latLon.lon - points[i - 1].m_latLon.lon,
                                         kMinDeltaLon, kMaxDeltaLon);

      WriteVarUint(bufWriter, deltaTimestamp);
      WriteVarUint(bufWriter, deltaLat);
      WriteVarUint(bufWriter, deltaLon);
    }

    auto const endPos = bufWriter.Pos();
    ASSERT_LESS(endPos - payloadStartPos, static_cast<uint32_t>(1) << 29, ("Payload too big."));
    ASSERT_LESS(version, static_cast<uint32_t>(1) << 3, ("Version too big."));
    uint32_t size = static_cast<uint32_t>(endPos - payloadStartPos);
    header = (size << 3) | version;
    bufWriter.Seek(startPos);
    bufWriter.Write(&header, sizeof(header));
    bufWriter.Seek(endPos);

    writer.Write(buf.data(), buf.size());
  }

  // Deserializes the points from |source| and appends them to |result|.
  // Version 0:
  //   The source must contain the encoded payload and nothing more (i.e.,
  //   the header must be read separately).
  template <typename Source>
  static void DeserializeDataPoints(uint32_t version, Source & src, vector<DataPoint> & result)
  {
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
