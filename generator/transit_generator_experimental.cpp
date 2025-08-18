#include "generator/transit_generator_experimental.hpp"

#include "generator/utils.hpp"

#include "traffic/traffic_cache.hpp"

#include "routing/index_router.hpp"
#include "routing/road_graph.hpp"
#include "routing/routing_exceptions.hpp"
#include "routing/vehicle_mask.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/routing_helpers.hpp"

#include "transit/experimental/transit_types_experimental.hpp"

#include "indexer/mwm_set.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <algorithm>
#include <memory>
#include <vector>

using namespace generator;
using namespace platform;
using namespace routing;
using namespace routing::transit;
using namespace storage;

namespace transit
{
namespace experimental
{
void FillOsmIdToFeatureIdsMap(std::string const & osmIdToFeatureIdsPath, OsmIdToFeatureIdsMap & mapping)
{
  bool const mappedIds =
      ForEachOsmId2FeatureId(osmIdToFeatureIdsPath, [&mapping](auto const & compositeId, auto featureId)
  { mapping[compositeId.m_mainId].push_back(featureId); });
  CHECK(mappedIds, (osmIdToFeatureIdsPath));
}

std::string GetMwmPath(std::string const & mwmDir, CountryId const & countryId)
{
  return base::JoinPath(mwmDir, countryId + DATA_FILE_EXTENSION);
}

std::vector<SingleMwmSegment> GetSegmentsFromEdges(std::vector<routing::Edge> && edges)
{
  std::vector<SingleMwmSegment> segments;
  segments.reserve(edges.size());

  for (auto const & edge : edges)
    segments.emplace_back(edge.GetFeatureId().m_index, edge.GetSegId(), edge.IsForward());

  return segments;
}

template <class D, class C>
std::vector<routing::Edge> GetBestPedestrianEdges(D const & destination, C && countryFileGetter,
                                                  IndexRouter & indexRouter, std::unique_ptr<WorldGraph> & worldGraph,
                                                  CountryId const & countryId)
{
  std::vector<routing::Edge> edges;

  if (countryFileGetter(destination.GetPoint()) != countryId)
    return edges;

  // For pedestrian routing all the segments are considered as two-way segments so
  // IndexRouter::FindBestSegments() finds the same segments for |isOutgoing| == true
  // and |isOutgoing| == false.
  try
  {
    if (countryFileGetter(destination.GetPoint()) != countryId)
      return edges;

    indexRouter.GetBestOutgoingEdges(destination.GetPoint(), *worldGraph, edges);
    return edges;
  }
  catch (MwmIsNotAliveException const & e)
  {
    LOG(LCRITICAL, ("Destination point", mercator::ToLatLon(destination.GetPoint()), "belongs to the mwm", countryId,
                    "but it's not alive:", e.what()));
  }
  catch (RootException const & e)
  {
    LOG(LCRITICAL, ("Exception while looking for the best segment in mwm", countryId, "Destination",
                    mercator::ToLatLon(destination.GetPoint()), e.what()));
  }

  return edges;
}

/// \brief Calculates best pedestrians segments for gates and stops which are imported from GTFS.
void CalculateBestPedestrianSegments(std::string const & mwmPath, CountryId const & countryId,
                                     TransitData & transitData)
{
  // Creating IndexRouter.
  SingleMwmDataSource dataSource(mwmPath);

  auto infoGetter = storage::CountryInfoReader::CreateCountryInfoGetter(GetPlatform());
  CHECK(infoGetter, ());

  auto const countryFileGetter = [&infoGetter](m2::PointD const & pt) { return infoGetter->GetRegionCountryId(pt); };

  auto const getMwmRectByName = [&](std::string const & c)
  {
    CHECK_EQUAL(countryId, c, ());
    return infoGetter->GetLimitRectForLeaf(c);
  };

  CHECK_EQUAL(dataSource.GetMwmId().GetInfo()->GetType(), MwmInfo::COUNTRY, ());
  auto numMwmIds = std::make_shared<NumMwmIds>();
  numMwmIds->RegisterFile(CountryFile(countryId));

  // |indexRouter| is valid while |dataSource| is valid.
  IndexRouter indexRouter(VehicleType::Pedestrian, false /* load altitudes */, CountryParentNameGetterFn(),
                          countryFileGetter, getMwmRectByName, numMwmIds, MakeNumMwmTree(*numMwmIds, *infoGetter),
                          traffic::TrafficCache(), dataSource.GetDataSource());
  auto worldGraph = indexRouter.MakeSingleMwmWorldGraph();

  auto const & gates = transitData.GetGates();
  for (size_t i = 0; i < gates.size(); ++i)
  {
    auto edges = GetBestPedestrianEdges(gates[i], countryFileGetter, indexRouter, worldGraph, countryId);
    transitData.SetGatePedestrianSegments(i, GetSegmentsFromEdges(std::move(edges)));
  }

  auto const & stops = transitData.GetStops();
  for (size_t i = 0; i < stops.size(); ++i)
  {
    auto edges = GetBestPedestrianEdges(stops[i], countryFileGetter, indexRouter, worldGraph, countryId);
    transitData.SetStopPedestrianSegments(i, GetSegmentsFromEdges(std::move(edges)));
  }
}

void DeserializeFromJson(OsmIdToFeatureIdsMap const & mapping, std::string const & transitJsonsPath, TransitData & data)
{
  data.DeserializeFromJson(transitJsonsPath, mapping);
}

EdgeIdToFeatureId BuildTransit(std::string const & mwmDir, CountryId const & countryId,
                               std::string const & osmIdToFeatureIdsPath, std::string const & transitDir)
{
  LOG(LINFO, ("Building experimental transit section for", countryId, "mwmDir:", mwmDir));
  std::string const mwmPath = GetMwmPath(mwmDir, countryId);

  OsmIdToFeatureIdsMap mapping;
  FillOsmIdToFeatureIdsMap(osmIdToFeatureIdsPath, mapping);

  std::string const transitPath = base::JoinPath(transitDir, countryId);
  if (!Platform::IsFileExistsByFullPath(transitPath))
  {
    LOG(LWARNING, ("Path to experimental transit files doesn't exist:", transitPath));
    return {};
  }

  TransitData data;
  DeserializeFromJson(mapping, transitPath, data);

  // Transit section can be empty.
  if (data.IsEmpty())
  {
    LOG(LWARNING, ("Experimental transit data deserialized from jsons is empty:", transitPath));
    return {};
  }

  CalculateBestPedestrianSegments(mwmPath, countryId, data);

  data.Sort();

  data.CheckSorted();
  data.CheckValid();
  data.CheckUnique();

  // Transit graph numerates features according to their orderto their order..
  EdgeIdToFeatureId edgeToFeature;
  for (uint32_t i = 0; i < data.GetEdges().size(); ++i)
  {
    auto const & e = data.GetEdges()[i];
    EdgeId id(e.GetStop1Id(), e.GetStop2Id(), e.GetLineId());
    edgeToFeature[id] = i;
  }

  FilesContainerW container(mwmPath, FileWriter::OP_WRITE_EXISTING);
  auto writer = container.GetWriter(TRANSIT_FILE_TAG);
  data.Serialize(*writer);
  return edgeToFeature;
}
}  // namespace experimental
}  // namespace transit
