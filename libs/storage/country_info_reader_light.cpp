#include "storage/country_info_reader_light.hpp"

#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

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
    ReaderSource<ModelReaderPtr> src(m_reader->GetReader(PACKED_POLYGONS_INFO_TAG));
    rw::Read(src, m_countries);
  }
  catch (FileReader::Exception const & exception)
  {
    LOG(LERROR, ("Exception while reading file:", PACKED_POLYGONS_FILE, "reason:", exception.what()));

    m_reader.reset();
    m_countries.clear();
  }

  m_nameGetter.SetLocale(languages::GetCurrentTwine());
}

std::vector<m2::RegionD> CountryInfoReader::LoadRegionsFromDisk(size_t id) const
{
  std::vector<m2::RegionD> result;
  ReaderSource<ModelReaderPtr> src(m_reader->GetReader(strings::to_string(id)));

  uint32_t const count = ReadVarUint<uint32_t>(src);
  for (size_t i = 0; i < count; ++i)
  {
    std::vector<m2::PointD> points;
    serial::LoadOuterPath(src, serial::GeometryCodingParams(), points);
    result.emplace_back(std::move(points));
  }
  return result;
}

bool CountryInfoReader::BelongsToRegion(m2::PointD const & pt, size_t id) const
{
  if (!m_countries[id].m_rect.IsPointInside(pt))
    return false;

  for (auto const & region : LoadRegionsFromDisk(id))
    if (region.Contains(pt))
      return true;

  return false;
}

CountryInfoReader::Info CountryInfoReader::GetMwmInfo(m2::PointD const & pt) const
{
  Info info;
  info.m_id = GetRegionCountryId(pt);
  info.m_name = m_nameGetter(info.m_id);
  return info;
}
}  // namespace lightweight
