#include "generator/transit_generator_experimental.hpp"

#include "generator/utils.hpp"

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

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/region2d.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <algorithm>
#include <vector>

#include "3party/jansson/src/jansson.h"

using namespace generator;
using namespace platform;
using namespace routing;
using namespace routing::transit;
using namespace std;
using namespace storage;

namespace transit
{
namespace experimental
{
void FillOsmIdToFeatureIdsMap(string const & osmIdToFeatureIdsPath, OsmIdToFeatureIdsMap & mapping)
{
  bool const mappedIds = ForEachOsmId2FeatureId(
      osmIdToFeatureIdsPath, [&mapping](auto const & compositeId, auto featureId) {
        mapping[compositeId.m_mainId].push_back(featureId);
      });
  CHECK(mappedIds, (osmIdToFeatureIdsPath));
}

string GetMwmPath(string const & mwmDir, CountryId const & countryId)
{
  return base::JoinPath(mwmDir, countryId + DATA_FILE_EXTENSION);
}

/// \brief Calculates best pedestrian segment for every gate in |graphData.m_gates|.
/// The result of the calculation is set to |Gate::m_bestPedestrianSegment| of every gate
/// from |graphData.m_gates|.
/// \note All gates in |graphData.m_gates| must have a valid |m_point| field before the call.
void CalculateBestPedestrianSegments(string const & mwmPath, CountryId const & countryId,
                                     TransitData & graphData)
{
  // TODO(o.khlopkova) Find best segments for gates and not-subway stops.
}

void DeserializeFromJson(OsmIdToFeatureIdsMap const & mapping, std::string const & transitJsonsPath,
                         TransitData & data)
{
  data.DeserializeFromJson(transitJsonsPath, mapping);
}

void ProcessData(string const & mwmPath, CountryId const & countryId,
                 OsmIdToFeatureIdsMap const & osmIdToFeatureIdsMap, TransitData & data)
{
  CalculateBestPedestrianSegments(mwmPath, countryId, data);
  data.Sort();
  data.CheckValidSortedUnique();
}

void BuildTransit(string const & mwmDir, CountryId const & countryId,
                  string const & osmIdToFeatureIdsPath, string const & transitDir)
{
  LOG(LINFO, ("Building experimental transit section for", countryId, "mwmDir:", mwmDir));
  string const mwmPath = GetMwmPath(mwmDir, countryId);
  OsmIdToFeatureIdsMap mapping;
  FillOsmIdToFeatureIdsMap(osmIdToFeatureIdsPath, mapping);

  TransitData data;
  DeserializeFromJson(mapping, base::JoinPath(transitDir, countryId), data);

  // Transit section can be empty.
  if (data.IsEmpty())
    return;

  // TODO(o.khlopkova) Implement filling best pedestrian segments for gates and stops, check
  // if result data is not valid and serialize it to mwm transit section.
}
}  // namespace experimental
}  // namespace transit
