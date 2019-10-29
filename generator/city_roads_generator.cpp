#include "generator/city_roads_generator.hpp"

#include "generator/cities_boundaries_checker.hpp"
#include "generator/feature_builder.hpp"

#include "routing/city_roads_serialization.hpp"
#include "routing/routing_helpers.hpp"

#include "platform/platform.hpp"

#include "indexer/feature.hpp"
#include "indexer/feature_data.cpp"
#include "indexer/feature_processor.hpp"

#include "coding/read_write_utils.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"

#include <utility>

#include "defines.hpp"

using namespace generator;
using namespace std;

namespace
{
void LoadFeatureBuildersToVector(string const & boundariesPath,
                                 CitiesBoundariesChecker::CitiesBoundaries & result)
{
  if (!Platform::IsFileExistsByFullPath(boundariesPath))
  {
    LOG(LINFO, ("No info about city boundaries for routing. No such file:", boundariesPath));
    return;
  }

  FileReader reader(boundariesPath);
  ReaderSource<FileReader> source(reader);

  size_t n = 0;
  while (source.Size() > 0)
  {
    vector<m2::PointD> boundary;
    rw::ReadVectorOfPOD(source, boundary);
    result.emplace_back(boundary);
    ++n;
  }

  LOG(LINFO, ("Read:", n, "boundaries from:", boundariesPath));
}

/// \brief Fills |cityRoadFeatureIds| with road feature ids if more then
/// |kInCityPointsRatio| * <feature point number> points of the feature belongs to a city or a town
/// according to |table|.
vector<uint32_t> CalcRoadFeatureIds(string const & dataPath, string const & boundariesPath)
{

  CitiesBoundariesChecker::CitiesBoundaries citiesBoundaries;
  LoadFeatureBuildersToVector(boundariesPath, citiesBoundaries);
  CitiesBoundariesChecker const checker(citiesBoundaries);

  vector<uint32_t> cityRoadFeatureIds;
  ForEachFromDat(dataPath, [&cityRoadFeatureIds, &checker](FeatureType & ft, uint32_t fid) {
    if (!routing::IsCarRoad(TypesHolder(ft)))
      return;

    ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

    size_t inCityPointsCounter = 0;
    for (size_t i = 0; i < ft.GetPointsCount(); ++i)
    {
      if (checker.InCity(ft.GetPoint(i)))
        ++inCityPointsCounter;
    }

    double constexpr kInCityPointsRatio = 0.8;
    if (inCityPointsCounter > kInCityPointsRatio * ft.GetPointsCount())
      cityRoadFeatureIds.push_back(ft.GetID().m_index);
  });

  return cityRoadFeatureIds;
}
}  // namespace

namespace routing
{
void SerializeCityRoads(string const & dataPath, vector<uint32_t> && cityRoadFeatureIds)
{
  if (cityRoadFeatureIds.empty())
    return;

  FilesContainerW cont(dataPath, FileWriter::OP_WRITE_EXISTING);
  auto w = cont.GetWriter(CITY_ROADS_FILE_TAG);

  routing::CityRoadsSerializer::Serialize(*w, move(cityRoadFeatureIds));
}

bool BuildCityRoads(string const & mwmPath, string const & boundariesPath)
{
  vector<uint32_t> cityRoadFeatureIds;
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
    cityRoadFeatureIds = CalcRoadFeatureIds(mwmPath, boundariesPath);
    SerializeCityRoads(mwmPath, move(cityRoadFeatureIds));
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error while building section city_roads in", mwmPath, ". Message:", e.Msg()));
    return false;
  }
  return true;
}
}  // namespace routing
