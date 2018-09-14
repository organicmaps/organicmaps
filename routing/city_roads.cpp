#include "routing/city_roads.hpp"

#include "routing/city_roads_serialization.hpp"

#include "indexer/data_source.hpp"

#include "coding/reader.hpp"

#include "base/logging.hpp"

#include <utility>

#include "defines.hpp"

namespace routing
{
bool CityRoads::IsCityRoad(uint32_t fid) const
{
  if (fid < m_cityRoads.size())
    return m_cityRoads[fid];

  return false;
}

bool LoadCityRoads(DataSource const & dataSource, MwmSet::MwmId const & mwmId, CityRoads & cityRoads)
{
  MwmSet::MwmHandle m_handle(dataSource.GetMwmHandleById(mwmId));
  if (!m_handle.IsAlive())
    return false;

  auto const & mwmValue = *m_handle.GetValue<MwmValue>();
  if (!mwmValue.m_cont.IsExist(CITY_ROADS_FILE_TAG))
    return false;

  try
  {
    FilesContainerR::TReader reader(mwmValue.m_cont.GetReader(CITY_ROADS_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(reader);
    CityRoadsSerializer::Deserialize(src, cityRoads.m_cityRoadsRegion, cityRoads.m_cityRoads);
    return true;
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("File", mwmValue.GetCountryFileName(), "Error while reading", CITY_ROADS_FILE_TAG,
        "section.", e.Msg()));
    return false;
  }
}
}  // namespace routing
