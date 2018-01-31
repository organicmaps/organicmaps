#include "indexer/locality_object.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/geometry_serialization.hpp"

#include "coding/byte_stream.hpp"

namespace indexer
{
void LocalityObject::Deserialize(char const * data)
{
  ArrayByteSource src(data);
  serial::CodingParams cp = {};
  ReadPrimitiveFromSource(src, m_id);
  uint8_t type;
  ReadPrimitiveFromSource(src, type);

  if (type == feature::GEOM_POINT)
  {
    m_points.push_back(serial::LoadPoint(src, cp));
    return;
  }

  ASSERT_EQUAL(type, feature::GEOM_AREA, ("Only supported types are GEOM_POINT and GEOM_AREA."));
  uint32_t trgCount;
  ReadPrimitiveFromSource(src, trgCount);
  CHECK_GREATER(trgCount, 0, ());
  trgCount += 2;
  char const * start = static_cast<char const *>(src.PtrC());
  serial::LoadInnerTriangles(start, trgCount, cp, m_triangles);
}
}  // namespace indexer
