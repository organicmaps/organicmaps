#pragma once

#include "generator/collector_interface.hpp"
#include "generator/feature_builder.hpp"
#include "generator/osm_element.hpp"
#include "generator/way_nodes_mapper.hpp"

#include "routing/road_access.hpp"
#include "routing/vehicle_mask.hpp"

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace routing
{
class OsmWay2FeaturePoint;

template <class Sink>
void Save(Sink & sink, RoadAccess::Type const & ac)
{
  WriteToSink(sink, static_cast<uint8_t>(ac));
}

template <class Source>
void Load(Source & src, RoadAccess::Type & ac)
{
  uint8_t const res = ReadPrimitiveFromSource<uint8_t>(src);
  CHECK_LESS(res, uint8_t(RoadAccess::Type::Count), ());
  ac = static_cast<RoadAccess::Type>(res);
}
}  // namespace routing

namespace routing_builder
{
using RoadAccess = routing::RoadAccess;
using VehicleType = routing::VehicleType;

using RoadAccessByVehicleType = std::array<RoadAccess, static_cast<size_t>(VehicleType::Count)>;

struct AccessConditional
{
  AccessConditional() = default;
  AccessConditional(RoadAccess::Type accessType, std::string const & openingHours)
    : m_accessType(accessType)
    , m_openingHours(openingHours)
  {}

  bool operator==(AccessConditional const & rhs) const
  {
    return std::tie(m_accessType, m_openingHours) == std::tie(rhs.m_accessType, rhs.m_openingHours);
  }

  friend std::string DebugPrint(AccessConditional const & ac)
  {
    return "AccessConditional { " + DebugPrint(ac.m_accessType) + ", " + ac.m_openingHours + " }";
  }

  RoadAccess::Type m_accessType = RoadAccess::Type::Count;
  std::string m_openingHours;
};

using ConditionalRAVectorT = std::vector<AccessConditional>;

template <class Sink>
void Save(Sink & sink, ConditionalRAVectorT const & ac)
{
  WriteToSink(sink, uint32_t(ac.size()));
  for (auto const & e : ac)
  {
    Save(sink, e.m_accessType);
    rw::WriteNonEmpty(sink, e.m_openingHours);
  }
}

template <class Source>
void Load(Source & src, ConditionalRAVectorT & vec)
{
  uint32_t const count = ReadPrimitiveFromSource<uint32_t>(src);
  vec.resize(count);
  for (uint32_t i = 0; i < count; ++i)
  {
    Load(src, vec[i].m_accessType);
    rw::ReadNonEmpty(src, vec[i].m_openingHours);
  }
}

class RoadAccessTagProcessor
{
public:
  using TagMapping = std::map<OsmElement::Tag, RoadAccess::Type>;
  using ConditionalTagsList = std::vector<std::string>;

  explicit RoadAccessTagProcessor(VehicleType vehicleType);

  void Process(OsmElement const & elem);
  void ProcessConditional(OsmElement const & elem);

  void SetIgnoreBarriers(OsmElement const & elem);
  bool IsIgnoreBarriers(size_t wayIdx) const;

  void MergeInto(RoadAccessTagProcessor & processor) const;

private:
  std::vector<TagMapping const *> m_barrierMappings;

  // Order of tag mappings in m_tagMappings is from more to less specific.
  // e.g. for car: motorcar, motorvehicle, vehicle, general access tags.
  std::vector<TagMapping const *> m_accessMappings;

  std::vector<ConditionalTagsList> m_conditionalTagsVector;

  // We decided to ignore some barriers without access on some type of highways
  // because we almost always do not need to add penalty for passes through such nodes.
  std::set<OsmElement::Tag> const * m_highwaysToIgnoreWABarriers = nullptr;
  std::vector<bool> m_ignoreWABarriers;

public:
  VehicleType m_vehicleType;

  generator::WayNodesMapper<RoadAccess::Type> m_barriersWithoutAccessTag;
  generator::WayNodesMapper<RoadAccess::Type> m_barriersWithAccessTag;
  generator::WaysMapper<RoadAccess::Type> m_wayToAccess;
  generator::WaysMapper<ConditionalRAVectorT> m_wayToAccessConditional;
};

class RoadAccessCollector : public generator::CollectorInterface
{
public:
  RoadAccessCollector(std::string const & filename, IDRInterfacePtr cache);

  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & cache = {}) const override;

  void CollectFeature(feature::FeatureBuilder const & fb, OsmElement const & elem) override;

  IMPLEMENT_COLLECTOR_IFACE(RoadAccessCollector);
  void MergeInto(RoadAccessCollector & collector) const;

protected:
  void Save() override;

private:
  IDRInterfacePtr m_cache;

  std::vector<RoadAccessTagProcessor> m_tagProcessors;

  generator::WaysIDHolder m_roads;
};

class AccessConditionalTagParser
{
public:
  static AccessConditionalTagParser const & Instance();

  AccessConditionalTagParser();
  std::vector<AccessConditional> ParseAccessConditionalTag(std::string const & tag, std::string const & value) const;

private:
  RoadAccess::Type GetAccessByVehicleAndStringValue(std::string const & vehicleFromTag,
                                                    std::string const & stringAccessValue) const;

  static std::optional<std::pair<size_t, std::string>> ReadUntilSymbol(std::string const & input, size_t startPos,
                                                                       char symbol);
  static std::string TrimAndDropAroundParentheses(std::string input);

  std::vector<RoadAccessTagProcessor::TagMapping> m_vehiclesToRoadAccess;
};

void ReadRoadAccess(std::string const & roadAccessPath, routing::OsmWay2FeaturePoint & way2feature,
                    RoadAccessByVehicleType & roadAccessByVehicleType);

// The generator tool's interface to writing the section with
// road accessibility information for one mwm file.
bool BuildRoadAccessInfo(std::string const & dataFilePath, std::string const & roadAccessPath,
                         routing::OsmWay2FeaturePoint & way2feature);
}  // namespace routing_builder
