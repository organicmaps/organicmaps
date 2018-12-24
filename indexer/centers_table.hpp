#pragma once

#include "coding/geometry_coding.hpp"
#include "coding/map_uint32_to_val.hpp"

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
  // Tries to get |center| of the feature identified by |id|.  Returns
  // false if table does not have entry for the feature.
  WARN_UNUSED_RESULT bool Get(uint32_t id, m2::PointD & center);

  // Loads CentersTable instance. Note that |reader| must be alive
  // until the destruction of loaded table. Returns nullptr if
  // CentersTable can't be loaded.
  static std::unique_ptr<CentersTable> Load(Reader & reader,
                                            serial::GeometryCodingParams const & codingParams);
private:
  using Map = MapUint32ToValue<m2::PointU>;

  bool Init(Reader & reader, serial::GeometryCodingParams const & codingParams);

  serial::GeometryCodingParams m_codingParams;
  std::unique_ptr<Map> m_map;
};

class CentersTableBuilder
{
public:
  inline void SetGeometryCodingParams(serial::GeometryCodingParams const & codingParams)
  {
    m_codingParams = codingParams;
  }

  void Put(uint32_t featureId, m2::PointD const & center);
  void Freeze(Writer & writer) const;

private:
  serial::GeometryCodingParams m_codingParams;
  MapUint32ToValueBuilder<m2::PointU> m_builder;
};
}  // namespace search
