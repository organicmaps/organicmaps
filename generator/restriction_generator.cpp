#include "generator/restriction_generator.hpp"

#include "generator/restriction_collector.hpp"

#include "routing/index_graph_loader.hpp"

#include "routing_common/car_model.cpp"
#include "routing_common/vehicle_model.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"

#include "coding/files_container.hpp"
#include "coding/file_writer.hpp"

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

namespace
{
using namespace routing;

std::unique_ptr<IndexGraph>
CreateIndexGraph(std::string const & targetPath,
                 std::string const & mwmPath,
                 std::string const & country,
                 CountryParentNameGetterFn const & countryParentNameGetterFn)
{
  std::shared_ptr<VehicleModelInterface> vehicleModel =
      CarModelFactory(countryParentNameGetterFn).GetVehicleModelForCountry(country);

  MwmValue mwmValue(
      platform::LocalCountryFile(targetPath, platform::CountryFile(country), 0 /* version */));

  auto graph = std::make_unique<IndexGraph>(
      std::make_shared<Geometry>(GeometryLoader::CreateFromFile(mwmPath, vehicleModel)),
      EdgeEstimator::Create(VehicleType::Car, *vehicleModel, nullptr /* trafficStash */,
        nullptr /* dataSource */, nullptr /* numMvmIds */));

  DeserializeIndexGraph(mwmValue, VehicleType::Car, *graph);

  return graph;
}
}  // namespace

namespace routing
{
std::unique_ptr<RestrictionCollector>
CreateRestrictionCollectorAndParse(
    std::string const & targetPath, std::string const & mwmPath, std::string const & country,
    std::string const & restrictionPath, std::string const & osmIdsToFeatureIdsPath,
    CountryParentNameGetterFn const & countryParentNameGetterFn)
{
  LOG(LDEBUG, ("BuildRoadRestrictions(", targetPath, ", ", restrictionPath, ", ",
                                         osmIdsToFeatureIdsPath, ");"));

  std::unique_ptr<IndexGraph> graph =
      CreateIndexGraph(targetPath, mwmPath, country, countryParentNameGetterFn);

  auto restrictionCollector =
      std::make_unique<RestrictionCollector>(osmIdsToFeatureIdsPath, std::move(graph));

  if (!restrictionCollector->Process(restrictionPath))
    return {};

  if (!restrictionCollector->HasRestrictions())
  {
    LOG(LINFO, ("No restrictions for", targetPath, "It's necessary to check that",
                restrictionPath, "and", osmIdsToFeatureIdsPath, "are available."));
    return {};
  }

  return restrictionCollector;
}

void SerializeRestrictions(RestrictionCollector & restrictionCollector,
                           std::string const & mwmPath)
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
      firstNextType =
          std::lower_bound(restrictions.cbegin(), restrictions.cend(),
                           Restriction(RestrictionHeader::kRestrictionTypes[i], {} /* links */),
                           base::LessBy(&Restriction::m_type));
    }
    else
    {
      firstNextType = restrictions.cend();
    }

    CHECK_GREATER_OR_EQUAL(i, 1, ("Unexpected overflow."));
    auto const prevType = RestrictionHeader::kRestrictionTypes[i - 1];
    header.SetNumberOf(prevType,
                       base::checked_cast<uint32_t>(std::distance(prevTypeEndIt, firstNextType)));

    prevTypeEndIt = firstNextType;
  }

  LOG(LINFO, ("Routing restriction info:", header));

  FilesContainerW cont(mwmPath, FileWriter::OP_WRITE_EXISTING);
  auto w = cont.GetWriter(RESTRICTIONS_FILE_TAG);
  header.Serialize(*w);

  base::SortUnique(restrictions);
  RestrictionSerializer::Serialize(header, restrictions.begin(), restrictions.end(), *w);
}

bool BuildRoadRestrictions(std::string const & targetPath,
                           std::string const & mwmPath,
                           std::string const & country,
                           std::string const & restrictionPath,
                           std::string const & osmIdsToFeatureIdsPath,
                           CountryParentNameGetterFn const & countryParentNameGetterFn)
{
  auto collector =
      CreateRestrictionCollectorAndParse(targetPath, mwmPath, country, restrictionPath,
                                         osmIdsToFeatureIdsPath, countryParentNameGetterFn);

  if (!collector)
    return false;

  SerializeRestrictions(*collector, mwmPath);
  return true;
}
}  // namespace routing
