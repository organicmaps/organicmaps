#pragma once

#include "coding/geometry_coding.hpp"

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
//
// *NOTE* This wrapper is abstract enough so feel free to change it,
// but note that there should always be backward-compatibility.  Thus,
// when adding new versions, never change data format of old versions.
//
// All centers tables are serialized in the following format:
//
// File offset (bytes)  Field name  Field size (bytes)
// 0                    version     2
// 2                    endianness  2
//
// Version and endianness is always in stored little-endian format.  0
// value of endianness means little endian, whereas 1 means big
// endian.
class CentersTable
{
public:
  struct Header
  {
    void Read(Reader & reader);
    void Write(Writer & writer);
    bool IsValid() const;

    uint16_t m_version = 0;
    uint16_t m_endianness = 0;
  };

  static_assert(sizeof(Header) == 4, "Wrong header size");

  virtual ~CentersTable() = default;

  // Tries to get |center| of the feature identified by |id|.  Returns
  // false if table does not have entry for the feature.
  WARN_UNUSED_RESULT virtual bool Get(uint32_t id, m2::PointD & center) = 0;

  // Loads CentersTable instance. Note that |reader| must be alive
  // until the destruction of loaded table. Returns nullptr if
  // CentersTable can't be loaded.
  static std::unique_ptr<CentersTable> Load(Reader & reader,
                                            serial::GeometryCodingParams const & codingParams);

private:
  virtual bool Init() = 0;
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

  std::vector<m2::PointU> m_centers;
  std::vector<uint32_t> m_ids;
};
}  // namespace search
