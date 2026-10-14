#include "storage/country_info_reader_light.hpp"

#include "platform/platform.hpp"

#include "coding/file_reader.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/read_write_utils.hpp"

#include "geometry/region2d.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include <vector>

#include "defines.hpp"

namespace lightweight
{
CountryInfoReader::CountryInfoReader()
{
  try
  {
    m_reader = std::make_unique<FilesContainerR>(GetPlatform().GetReader(PACKED_POLYGONS_FILE));
    ReaderSource src(m_reader->GetReader(PACKED_POLYGONS_INFO_TAG));
    rw::Read(src, m_countries);
  }
  catch (FileReader::Exception const & exception)
  {
    LOG(LERROR, ("Exception while reading file:", PACKED_POLYGONS_FILE, "reason:", exception.what()));

    m_reader.reset();
    m_countries.clear();
  }
}

bool CountryInfoReader::BelongsToRegion(m2::PointD const & pt, size_t id) const
{
  if (!m_countries[id].IsPointInsideRect(pt))
    return false;

  ReaderSource src(m_reader->GetReader(strings::to_string(id)));

  uint32_t const count = ReadVarUint<uint32_t>(src);
  for (size_t i = 0; i < count; ++i)
  {
    std::vector<m2::PointD> points;
    serial::LoadOuterPath(src, serial::GeometryCodingParams(), points);
    if (m2::RegionD(std::move(points)).Contains(pt))
      return true;
  }
  return false;
}
}  // namespace lightweight
