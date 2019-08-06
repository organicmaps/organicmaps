#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/intermediate_elements.hpp"
#include "generator/osm_element.hpp"

#include "routing/road_access.hpp"
#include "routing/vehicle_mask.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

struct OsmElement;
class FeatureParams;

namespace generator
{
namespace cache
{
class IntermediateDataReader;
}  // namespace cache
}  // namespace generator

// The road accessibility information is collected in the same manner
// as the restrictions are.
// See generator/restriction_generator.hpp for details.
namespace routing
{
class RoadAccessTagProcessor
{
public:
  using TagMapping = std::map<OsmElement::Tag, RoadAccess::Type>;

  explicit RoadAccessTagProcessor(VehicleType vehicleType);

  void Process(OsmElement const & elem);
  void WriteWayToAccess(std::ostream & stream);
  void WriteBarrierTags(std::ostream & stream, uint64_t id, std::vector<uint64_t> const & points);
  void Merge(RoadAccessTagProcessor const & roadAccessTagProcessor);

private:
  bool ShouldIgnoreBarrierWithoutAccess(OsmElement const & osmElement) const;
  RoadAccess::Type GetAccessType(OsmElement const & elem) const;

  VehicleType m_vehicleType;

  std::vector<TagMapping const *> m_barrierMappings;

  // Order of tag mappings in m_tagMappings is from more to less specific.
  // e.g. for car: motorcar, motorvehicle, vehicle, general access tags.
  std::vector<TagMapping const *> m_accessMappings;

  // We decided to ignore some barriers without access on some type of highways
  // because we almost always do not need to add penalty for passes through such nodes.
  std::set<OsmElement::Tag> const * m_hwIgnoreBarriersWithoutAccess;

  std::unordered_map<uint64_t, RoadAccess::Type> m_barriers;
  std::unordered_map<uint64_t, RoadAccess::Type> m_wayToAccess;
};

class RoadAccessWriter : public generator::CollectorInterface
{
public:
  explicit RoadAccessWriter(std::string const & filename);

  // CollectorInterface overrides:
  ~RoadAccessWriter() override;

  std::shared_ptr<CollectorInterface>
  Clone(std::shared_ptr<generator::cache::IntermediateDataReader> const & = {}) const override;

  void CollectFeature(feature::FeatureBuilder const & fb, OsmElement const & elem) override;
  void Finish() override;
  void Save() override;

  void Merge(generator::CollectorInterface const & collector) override;
  void MergeInto(RoadAccessWriter & collector) const override;

private:
  std::string m_waysFilename;
  std::unique_ptr<FileWriter> m_waysWriter;
  std::vector<RoadAccessTagProcessor> m_tagProcessors;
};

class RoadAccessCollector
{
public:
  using RoadAccessByVehicleType = std::array<RoadAccess, static_cast<size_t>(VehicleType::Count)>;

  RoadAccessCollector(std::string const & dataFilePath, std::string const & roadAccessPath,
                      std::string const & osmIdsToFeatureIdsPath);

  RoadAccessByVehicleType const & GetRoadAccessAllTypes() const
  {
    return m_roadAccessByVehicleType;
  }

  bool IsValid() const { return m_valid; }

private:
  RoadAccessByVehicleType m_roadAccessByVehicleType;
  bool m_valid = true;
};

// The generator tool's interface to writing the section with
// road accessibility information for one mwm file.
void BuildRoadAccessInfo(std::string const & dataFilePath, std::string const & roadAccessPath,
                         std::string const & osmIdsToFeatureIdsPath);
}  // namespace routing
