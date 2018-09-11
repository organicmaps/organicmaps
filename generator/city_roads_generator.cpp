#include "generator/city_roads_generator.hpp"

#include "generator/cities_boundaries_checker.hpp"

#include "routing/city_roads_serialization.hpp"
#include "routing/routing_helpers.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_data.cpp"
#include "indexer/feature_processor.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"

#include <utility>

#include "defines.hpp"

using namespace generator;
using namespace std;

namespace
{
void TableToVector(OsmIdToBoundariesTable & table,
                   CitiesBoundariesChecker::CitiesBoundaries & result)
{
  table.ForEachCluster([&result](vector<base::GeoObjectId> const & /* id */,
                                 vector<indexer::CityBoundary> const & cb) {
    result.insert(result.end(), cb.cbegin(), cb.cend());
  });
}

/// \brief Fills |cityRoadFeatureIds| with road feature ids if more then
/// |kInCityPointsRatio| * <feature point number> points of the feature belongs to a city or a town
/// according to |table|.
void CalcRoadFeatureIds(string const & dataPath, OsmIdToBoundariesTable & table,
                        vector<uint64_t> & cityRoadFeatureIds)
{
  double constexpr kInCityPointsRatio = 0.2;

  CitiesBoundariesChecker::CitiesBoundaries citiesBoundaries;
  TableToVector(table, citiesBoundaries);
  CitiesBoundariesChecker const checker(citiesBoundaries);

  ForEachFromDat(dataPath, [&cityRoadFeatureIds, &checker](FeatureType & ft, uint32_t fid) {
    if (!routing::IsRoad(TypesHolder(ft)))
      return;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    size_t inCityPointsCounter = 0;
    for (size_t i = 0; i < ft.GetPointsCount(); ++i)
    {
      if (checker.InCity(ft.GetPoint(i)))
        ++inCityPointsCounter;
    }

    if (inCityPointsCounter > kInCityPointsRatio * ft.GetPointsCount())
      cityRoadFeatureIds.push_back(ft.GetID().m_index);
  });
}
}  // namespace

namespace routing
{
void SerializeCityRoads(string const & dataPath, vector<uint64_t> && cityRoadFeatureIds)
{
  if (cityRoadFeatureIds.empty())
    return;

  FilesContainerW cont(dataPath, FileWriter::OP_WRITE_EXISTING);
  FileWriter w = cont.GetWriter(CITY_ROADS_FILE_TAG);

  routing::CityRoadsSerializer::Serialize(w, move(cityRoadFeatureIds));
}

bool BuildCityRoads(string const & dataPath, OsmIdToBoundariesTable & table)
{
  LOG(LINFO, ("BuildCityRoads(", dataPath, ");"));
  vector<uint64_t> cityRoadFeatureIds;
  try
  {
    // @TODO(bykoianko) The generation city roads section process is based on two stages now:
    // * dumping cities boundaries on feature generation step
    // * calculating feature ids and building section when feature ids are available
    // As a result of dumping cities boundaries instances of indexer::CityBoundary objects
    // are generated and dumped. These objects are used for generating city roads section.
    // Using real geometry of cities boundaries should be considered for generating city road
    // features. That mean that the real geometry of cities boundaries should be dumped
    // on the first step. And then to try using the real geometry should be used for generating city
    // road features. But there's a chance that it takes to long time.
    CalcRoadFeatureIds(dataPath, table, cityRoadFeatureIds);
    SerializeCityRoads(dataPath, move(cityRoadFeatureIds));
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while building section city_roads in", dataPath, ". Message:", e.Msg()));
    return false;
  }
  return true;
}
}  // namespace routing
