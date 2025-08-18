#pragma once

#include "coding/geometry_coding.hpp"
#include "coding/map_uint32_to_val.hpp"
#include "coding/point_coding.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <memory>
#include <vector>

class FilesContainerR;
class Reader;
class Writer;

namespace search
{
// A wrapper class around serialized centers-table.
class CentersTable
{
public:
  enum class Version : uint8_t
  {
    V0 = 0,
    V1 = 1,
    Latest = V1
  };

  struct Header
  {
    template <typename Sink>
    void Serialize(Sink & sink) const
    {
      CHECK_EQUAL(static_cast<uint8_t>(m_version), static_cast<uint8_t>(Version::V1), ());
      WriteToSink(sink, static_cast<uint8_t>(m_version));
      WriteToSink(sink, m_geometryParamsOffset);
      WriteToSink(sink, m_geometryParamsSize);
      WriteToSink(sink, m_centersOffset);
      WriteToSink(sink, m_centersSize);
    }

    void Read(Reader & reader);

    Version m_version = Version::Latest;
    // All offsets are relative to the start of the section (offset of header is zero).
    uint32_t m_geometryParamsOffset = 0;
    uint32_t m_geometryParamsSize = 0;
    uint32_t m_centersOffset = 0;
    uint32_t m_centersSize = 0;
  };

  // Tries to get |center| of the feature identified by |id|.  Returns
  // false if table does not have entry for the feature.
  [[nodiscard]] bool Get(uint32_t id, m2::PointD & center);

  uint64_t Count() const { return m_map->Count(); }

  // Loads CentersTable instance. Note that |reader| must be alive
  // until the destruction of loaded table. Returns nullptr if
  // CentersTable can't be loaded.
  static std::unique_ptr<CentersTable> LoadV0(Reader & reader, serial::GeometryCodingParams const & codingParams);

  static std::unique_ptr<CentersTable> LoadV1(Reader & reader);

private:
  using Map = MapUint32ToValue<m2::PointU>;

  bool Init(Reader & reader, serial::GeometryCodingParams const & codingParams, m2::RectD const & limitRect);

  serial::GeometryCodingParams m_codingParams;
  std::unique_ptr<Map> m_map;
  std::unique_ptr<Reader> m_centersSubreader;
  m2::RectD m_limitRect;
  Version m_version = Version::Latest;
};

class CentersTableBuilder
{
public:
  void SetGeometryParams(m2::RectD const & limitRect, double pointAccuracy = kMwmPointAccuracy);
  void Put(uint32_t featureId, m2::PointD const & center);
  void WriteBlock(Writer & w, std::vector<m2::PointU>::const_iterator begin,
                  std::vector<m2::PointU>::const_iterator end) const;
  void Freeze(Writer & writer) const;

  void SetGeometryCodingParamsV0ForTests(serial::GeometryCodingParams const & codingParams);
  void PutV0ForTests(uint32_t featureId, m2::PointD const & center);
  void FreezeV0ForTests(Writer & writer) const;

private:
  serial::GeometryCodingParams m_codingParams;
  m2::RectD m_limitRect;
  MapUint32ToValueBuilder<m2::PointU> m_builder;
};
}  // namespace search
