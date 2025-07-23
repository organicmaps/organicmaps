#include "generator/restriction_generator.hpp"

#include "generator/restriction_collector.hpp"

#include "routing/index_graph_loader.hpp"

#include "routing_common/car_model.hpp"
#include "routing_common/vehicle_model.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"

#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"

#include "base/checked_cast.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include "defines.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace routing_builder
{
using namespace routing;

std::unique_ptr<IndexGraph> CreateIndexGraph(std::string const & mwmPath, std::string const & country,
                                             CountryParentNameGetterFn const & countryParentNameGetterFn)
{
  std::shared_ptr<VehicleModelInterface> vehicleModel =
      CarModelFactory(countryParentNameGetterFn).GetVehicleModelForCountry(country);

  MwmValue mwmValue(platform::LocalCountryFile::MakeTemporary(mwmPath));

  auto graph =
      std::make_unique<IndexGraph>(std::make_shared<Geometry>(GeometryLoader::CreateFromFile(mwmPath, vehicleModel)),
                                   EdgeEstimator::Create(VehicleType::Car, *vehicleModel, nullptr /* trafficStash */,
                                                         nullptr /* dataSource */, nullptr /* numMvmIds */));

  DeserializeIndexGraph(mwmValue, VehicleType::Car, *graph);
  return graph;
}

void SerializeRestrictions(RestrictionCollector & restrictionCollector, std::string const & mwmPath)
{
  std::vector<Restriction> restrictions = restrictionCollector.StealRestrictions();
  CHECK(std::is_sorted(restrictions.cbegin(), restrictions.cend()), ());

  RestrictionHeader header;

  auto prevTypeEndIt = restrictions.cbegin();
  for (size_t i = 1; i <= RestrictionHeader::kRestrictionTypes.size(); ++i)
  {
    decltype(restrictions.cbegin()) firstNextType;
    if (i != RestrictionHeader::kRestrictionTypes.size())
    {
      firstNextType = std::lower_bound(restrictions.cbegin(), restrictions.cend(),
                                       Restriction(RestrictionHeader::kRestrictionTypes[i], {} /* links */),
                                       base::LessBy(&Restriction::m_type));
    }
    else
    {
      firstNextType = restrictions.cend();
    }

    CHECK_GREATER_OR_EQUAL(i, 1, ("Unexpected overflow."));
    auto const prevType = RestrictionHeader::kRestrictionTypes[i - 1];
    header.SetNumberOf(prevType, base::checked_cast<uint32_t>(std::distance(prevTypeEndIt, firstNextType)));

    prevTypeEndIt = firstNextType;
  }

  LOG(LINFO, ("Routing restriction info:", header));

  FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
  auto w = cont.GetWriter(RESTRICTIONS_FILE_TAG);
  header.Serialize(*w);

  base::SortUnique(restrictions);
  RestrictionSerializer::Serialize(header, restrictions.begin(), restrictions.end(), *w);
}

bool BuildRoadRestrictions(IndexGraph & graph, std::string const & mwmPath, std::string const & restrictionPath,
                           std::string const & osmIdsToFeatureIdsPath)
{
  LOG(LINFO, ("Generating restrictions for", restrictionPath));

  auto collector = std::make_unique<RestrictionCollector>(osmIdsToFeatureIdsPath, graph);
  if (!collector->Process(restrictionPath))
    return false;

  if (!collector->HasRestrictions())
  {
    LOG(LWARNING,
        ("No restrictions created. Check that", restrictionPath, "and", osmIdsToFeatureIdsPath, "are available."));
    return false;
  }

  SerializeRestrictions(*collector, mwmPath);
  return true;
}

}  // namespace routing_builder
