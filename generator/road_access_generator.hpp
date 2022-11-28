#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/intermediate_elements.hpp"
#include "generator/osm_element.hpp"

#include "routing/road_access.hpp"
#include "routing/vehicle_mask.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>


// The road accessibility information is collected in the same manner
// as the restrictions are.
// See generator/restriction_generator.hpp for details.
namespace routing_builder
{
using RoadAccess = routing::RoadAccess;
using VehicleType = routing::VehicleType;

struct AccessConditional
{
  AccessConditional() = default;
  AccessConditional(RoadAccess::Type accessType, std::string const & openingHours)
      : m_accessType(accessType), m_openingHours(openingHours)
  {
  }

  bool operator==(AccessConditional const & rhs) const
  {
    return std::tie(m_accessType, m_openingHours) == std::tie(rhs.m_accessType, rhs.m_openingHours);
  }

  RoadAccess::Type m_accessType = RoadAccess::Type::Count;
  std::string m_openingHours;
};

class RoadAccessTagProcessor
{
public:
  using TagMapping = std::map<OsmElement::Tag, RoadAccess::Type>;
  using ConditionalTagsList = std::vector<std::string>;

  explicit RoadAccessTagProcessor(VehicleType vehicleType);

  void Process(OsmElement const & elem);
  void ProcessConditional(OsmElement const & elem);
  void WriteWayToAccess(std::ostream & stream);
  void WriteWayToAccessConditional(std::ostream & stream);
  void WriteBarrierTags(std::ostream & stream, uint64_t id, std::vector<uint64_t> const & points,
                        bool ignoreBarrierWithoutAccess);
  void Merge(RoadAccessTagProcessor const & roadAccessTagProcessor);

private:
  VehicleType m_vehicleType;

  std::vector<TagMapping const *> m_barrierMappings;

  // Order of tag mappings in m_tagMappings is from more to less specific.
  // e.g. for car: motorcar, motorvehicle, vehicle, general access tags.
  std::vector<TagMapping const *> m_accessMappings;

  std::vector<ConditionalTagsList> m_conditionalTagsVector;

  // We decided to ignore some barriers without access on some type of highways
  // because we almost always do not need to add penalty for passes through such nodes.
  std::optional<std::set<OsmElement::Tag>> m_hwIgnoreBarriersWithoutAccess;

  std::unordered_map<uint64_t, RoadAccess::Type> m_barriersWithoutAccessTag;
  std::unordered_map<uint64_t, RoadAccess::Type> m_barriersWithAccessTag;
  std::unordered_map<uint64_t, RoadAccess::Type> m_wayToAccess;
  std::unordered_map<uint64_t, std::vector<AccessConditional>> m_wayToAccessConditional;
};

class RoadAccessWriter : public generator::CollectorInterface
{
public:
  explicit RoadAccessWriter(std::string const & filename);

  // CollectorInterface overrides:
  ~RoadAccessWriter() override;

  std::shared_ptr<CollectorInterface> Clone(
      std::shared_ptr<generator::cache::IntermediateDataReaderInterface> const &) const override;

  void CollectFeature(feature::FeatureBuilder const & fb, OsmElement const & elem) override;
  void Finish() override;

  void Merge(generator::CollectorInterface const & collector) override;
  void MergeInto(RoadAccessWriter & collector) const override;

protected:
  void Save() override;
  void OrderCollectedData() override;

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

class AccessConditionalTagParser
{
public:
  static AccessConditionalTagParser const & Instance();

  AccessConditionalTagParser();
  std::vector<AccessConditional> ParseAccessConditionalTag(
      std::string const & tag, std::string const & value) const;

private:
  RoadAccess::Type GetAccessByVehicleAndStringValue(std::string const & vehicleFromTag,
                                                    std::string const & stringAccessValue) const;

  static std::optional<std::pair<size_t, std::string>> ReadUntilSymbol(std::string const & input,
                                                                size_t startPos, char symbol) ;
  static std::string TrimAndDropAroundParentheses(std::string input) ;

  std::vector<RoadAccessTagProcessor::TagMapping> m_vehiclesToRoadAccess;
};

// The generator tool's interface to writing the section with
// road accessibility information for one mwm file.
bool BuildRoadAccessInfo(std::string const & dataFilePath, std::string const & roadAccessPath,
                         std::string const & osmIdsToFeatureIdsPath);
}  // namespace routing_builder
