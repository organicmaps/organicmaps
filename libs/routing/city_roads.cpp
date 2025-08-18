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

void CityRoads::Load(ReaderT const & reader)
{
  ReaderSource<ReaderT> src(reader);
  CityRoadsSerializer::Deserialize(src, m_cityRoadsRegion, m_cityRoads);
}

std::unique_ptr<CityRoads> LoadCityRoads(MwmSet::MwmHandle const & handle)
{
  auto const * value = handle.GetValue();
  CHECK(value, ());

  try
  {
    auto cityRoads = std::make_unique<CityRoads>();
    if (value->m_cont.IsExist(CITY_ROADS_FILE_TAG))
      cityRoads->Load(value->m_cont.GetReader(CITY_ROADS_FILE_TAG));
    return cityRoads;
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("File", value->GetCountryFileName(), "Error while reading", CITY_ROADS_FILE_TAG, "section.", e.Msg()));
    return std::make_unique<CityRoads>();
  }
}
}  // namespace routing
