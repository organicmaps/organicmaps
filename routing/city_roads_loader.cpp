#include "routing/city_roads_loader.hpp"

#include "routing/city_roads_serialization.hpp"

#include "indexer/data_source.hpp"

#include "coding/reader.hpp"

#include "base/logging.hpp"

#include <utility>

#include "defines.hpp"

namespace routing
{
CityRoadsLoader::CityRoadsLoader(DataSource const & dataSource, MwmSet::MwmId const & mwmId)
{
  MwmSet::MwmHandle m_handle(dataSource.GetMwmHandleById(mwmId));
  if (!m_handle.IsAlive())
    return;

  auto const & mwmValue = *m_handle.GetValue<MwmValue>();
  if (!mwmValue.m_cont.IsExist(CITY_ROADS_FILE_TAG))
    return;

  try
  {
    FilesContainerR::TReader reader(mwmValue.m_cont.GetReader(CITY_ROADS_FILE_TAG));
    ReaderSource<FilesContainerR::TReader> src(reader);
    CityRoadsSerializer::Deserialize(src, m_cityRoadsRegion, m_cityRoads);
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("File", mwmValue.GetCountryFileName(), "Error while reading", CITY_ROADS_FILE_TAG,
                 "section.", e.Msg()));
  }
}

bool CityRoadsLoader::IsCityRoad(uint32_t fid) const
{
  if (fid < m_cityRoads.size())
    return m_cityRoads[fid];

  return false;
}
}  // namespace routing
