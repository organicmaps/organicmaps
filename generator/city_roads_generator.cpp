#include "generator/city_roads_generator.hpp"

#include "generator/cities_boundaries_checker.hpp"
#include "generator/collector_routing_city_boundaries.hpp"

#include "routing/city_roads_serialization.hpp"
#include "routing/routing_helpers.hpp"

#include "platform/platform.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"

#include "geometry/circle_on_earth.hpp"

#include "base/logging.hpp"

#include "defines.hpp"

namespace routing_builder
{
using generator::CitiesBoundariesChecker;
using std::string, std::vector;

void LoadCitiesBoundariesGeometry(string const & boundariesPath, CitiesBoundariesChecker::CitiesBoundaries & result)
{
  if (!Platform::IsFileExistsByFullPath(boundariesPath))
  {
    LOG(LWARNING, ("No city boundaries file:", boundariesPath));
    return;
  }

  generator::PlaceBoundariesHolder holder;
  holder.Deserialize(boundariesPath);

  size_t points = 0, areas = 0;
  holder.ForEachLocality([&](generator::PlaceBoundariesHolder::Locality & loc)
  {
    CHECK(loc.TestValid(), ());

    if (loc.IsPoint() || !loc.IsHonestCity())
    {
      ++points;
      double const radiusM = ftypes::GetRadiusByPopulationForRouting(loc.GetPopulation(), loc.GetPlace());
      result.emplace_back(
          ms::CreateCircleGeometryOnEarth(mercator::ToLatLon(loc.m_center), radiusM, 30.0 /* angleStepDegree */));
    }
    else
    {
      ++areas;
      for (auto & poly : loc.m_boundary)
        result.emplace_back(std::move(poly));
    }
  });

  LOG(LINFO, ("Read", points, "point places and", areas, "area places from:", boundariesPath));
}

/// \brief Fills |cityRoadFeatureIds| with road feature ids if more then
/// |kInCityPointsRatio| * <feature point number> points of the feature belongs to a city or a town
/// according to |table|.
vector<uint32_t> CalcRoadFeatureIds(string const & dataPath, string const & boundariesPath)
{
  CitiesBoundariesChecker::CitiesBoundaries citiesBoundaries;
  LoadCitiesBoundariesGeometry(boundariesPath, citiesBoundaries);
  CitiesBoundariesChecker const checker(citiesBoundaries);

  vector<uint32_t> cityRoadFeatureIds;
  feature::ForEachFeature(dataPath, [&cityRoadFeatureIds, &checker](FeatureType & ft, uint32_t)
  {
    feature::TypesHolder types(ft);
    if (!routing::IsCarRoad(types) && !routing::IsBicycleRoad(types))
      return;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    size_t inCityPointsCounter = 0;
    size_t const count = ft.GetPointsCount();
    for (size_t i = 0; i < count; ++i)
      if (checker.InCity(ft.GetPoint(i)))
        ++inCityPointsCounter;

    // Our approximation of boundary overestimates it, because of different
    // bounding boxes (in order to increase performance). So we don't want
    // match some long roads as city roads, because they pass near city, but
    // not though it.
    double constexpr kPointsRatioInCity = 0.8;
    if (inCityPointsCounter > kPointsRatioInCity * count)
      cityRoadFeatureIds.push_back(ft.GetID().m_index);
  });

  return cityRoadFeatureIds;
}

void SerializeCityRoads(string const & dataPath, vector<uint32_t> && cityRoadFeatureIds)
{
  if (cityRoadFeatureIds.empty())
    return;

  FilesContainerW cont(dataPath, FileWriter::OP_WRITE_EXISTING);
  auto w = cont.GetWriter(CITY_ROADS_FILE_TAG);

  routing::CityRoadsSerializer::Serialize(*w, std::move(cityRoadFeatureIds));
}

bool BuildCityRoads(string const & mwmPath, string const & boundariesPath)
{
  try
  {
    // The generation city roads section process is based on two stages now:
    // * dumping cities boundaries on feature generation step
    // * calculating feature ids and building section when feature ids are available
    // As a result of dumping cities boundaries instances of indexer::CityBoundary objects
    // are generated and dumped. These objects are used for generating city roads section.
    SerializeCityRoads(mwmPath, CalcRoadFeatureIds(mwmPath, boundariesPath));
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while building section city_roads in", mwmPath, ". Message:", e.Msg()));
    return false;
  }
  return true;
}
}  // namespace routing_builder
