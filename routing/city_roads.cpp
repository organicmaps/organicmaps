#include "routing/city_roads.hpp"

#include "routing/city_roads_serialization.hpp"

#include "indexer/data_source.hpp"

#include "base/logging.hpp"

#include <utility>

#include "defines.hpp"

namespace routing
{
bool CityRoads::IsCityRoad(uint32_t fid) const
{
  return fid < m_cityRoads.size() ? m_cityRoads[fid] : false;
}

void LoadCityRoads(std::string const & fileName, FilesContainerR::TReader const & reader,
                   CityRoads & cityRoads)
{
  try
  {
    ReaderSource<FilesContainerR::TReader> src(reader);
    CityRoadsSerializer::Deserialize(src, cityRoads.m_cityRoadsRegion, cityRoads.m_cityRoads);
  }
  catch (Reader::OpenException const & e)
  {
    LOG(LERROR, ("File", fileName, "Error while reading", CITY_ROADS_FILE_TAG,
        "section.", e.Msg()));
  }
}

std::unique_ptr<CityRoads> LoadCityRoads(DataSource const & dataSource,
                                         MwmSet::MwmHandle const & handle)
{
  auto cityRoads = std::make_unique<CityRoads>();
  auto const & mwmValue = *handle.GetValue();
  if (!mwmValue.m_cont.IsExist(CITY_ROADS_FILE_TAG))
    return cityRoads;

  LoadCityRoads(mwmValue.GetCountryFileName(), mwmValue.m_cont.GetReader(CITY_ROADS_FILE_TAG),
                *cityRoads);
  return cityRoads;
}
}  // namespace routing
