#include "generator/road_access_generator.hpp"

#include "generator/routing_helpers.hpp"

#include "routing/road_access.hpp"
#include "routing/road_access_serialization.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/features_vector.hpp"

#include "coding/file_container.hpp"
#include "coding/file_writer.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"
#include "base/osm_id.hpp"

#include <initializer_list>

#include "defines.hpp"

#include <algorithm>
#include <utility>

using namespace routing;
using namespace std;

namespace
{
char constexpr kDelim[] = " \t\r\n";

using TagMapping = routing::RoadAccessTagProcessor::TagMapping;

TagMapping const kMotorCarTagMapping = {
    {OsmElement::Tag("motorcar", "yes"), RoadAccess::Type::Yes},
    {OsmElement::Tag("motorcar", "no"), RoadAccess::Type::No},
    {OsmElement::Tag("motorcar", "private"), RoadAccess::Type::Private},
    {OsmElement::Tag("motorcar", "destination"), RoadAccess::Type::Destination},
};

TagMapping const kMotorVehicleTagMapping = {
    {OsmElement::Tag("motor_vehicle", "yes"), RoadAccess::Type::Yes},
    {OsmElement::Tag("motor_vehicle", "no"), RoadAccess::Type::No},
    {OsmElement::Tag("motor_vehicle", "private"), RoadAccess::Type::Private},
    {OsmElement::Tag("motor_vehicle", "destination"), RoadAccess::Type::Destination},
};

TagMapping const kVehicleTagMapping = {
    {OsmElement::Tag("vehicle", "yes"), RoadAccess::Type::Yes},
    {OsmElement::Tag("vehicle", "no"), RoadAccess::Type::No},
    {OsmElement::Tag("vehicle", "private"), RoadAccess::Type::Private},
    {OsmElement::Tag("vehicle", "destination"), RoadAccess::Type::Destination},
};

TagMapping const kCarBarriersTagMapping = {
    {OsmElement::Tag("barrier", "block"), RoadAccess::Type::No},
    {OsmElement::Tag("barrier", "bollard"), RoadAccess::Type::No},
    {OsmElement::Tag("barrier", "chain"), RoadAccess::Type::No},
    {OsmElement::Tag("barrier", "cycle_barrier"), RoadAccess::Type::No},
    {OsmElement::Tag("barrier", "gate"), RoadAccess::Type::Private},
    {OsmElement::Tag("barrier", "jersey_barrier"), RoadAccess::Type::No},
    {OsmElement::Tag("barrier", "lift_gate"), RoadAccess::Type::Private},
    {OsmElement::Tag("barrier", "log"), RoadAccess::Type::No},
    {OsmElement::Tag("barrier", "motorcycle_barrier"), RoadAccess::Type::No},
    {OsmElement::Tag("barrier", "swing_gate"), RoadAccess::Type::Private},
};

TagMapping const kPedestrianTagMapping = {
    {OsmElement::Tag("foot", "yes"), RoadAccess::Type::Yes},
    {OsmElement::Tag("foot", "no"), RoadAccess::Type::No},
    {OsmElement::Tag("foot", "private"), RoadAccess::Type::Private},
    {OsmElement::Tag("foot", "destination"), RoadAccess::Type::Destination},
};

TagMapping const kBicycleTagMapping = {
    {OsmElement::Tag("bicycle", "yes"), RoadAccess::Type::Yes},
    {OsmElement::Tag("bicycle", "no"), RoadAccess::Type::No},
    {OsmElement::Tag("bicycle", "private"), RoadAccess::Type::Private},
    {OsmElement::Tag("bicycle", "destination"), RoadAccess::Type::Destination},
};

TagMapping const kBicycleBarriersTagMapping = {
    {OsmElement::Tag("barrier", "cycle_barrier"), RoadAccess::Type::No},
    {OsmElement::Tag("barrier", "turnstile"), RoadAccess::Type::No},
    {OsmElement::Tag("barrier", "kissing_gate"), RoadAccess::Type::Private},
    {OsmElement::Tag("barrier", "gate"), RoadAccess::Type::Private},
};

// Allow everything to keep transit section empty. We'll use pedestrian section for
// transit + pedestrian combination.
TagMapping const kTransitTagMapping = {
    {OsmElement::Tag("access", "yes"), RoadAccess::Type::Yes},
    {OsmElement::Tag("access", "no"), RoadAccess::Type::Yes},
    {OsmElement::Tag("access", "private"), RoadAccess::Type::Yes},
    {OsmElement::Tag("access", "destination"), RoadAccess::Type::Yes},
};

TagMapping const kDefaultTagMapping = {
    {OsmElement::Tag("access", "yes"), RoadAccess::Type::Yes},
    {OsmElement::Tag("access", "no"), RoadAccess::Type::No},
    {OsmElement::Tag("access", "private"), RoadAccess::Type::Private},
    {OsmElement::Tag("access", "destination"), RoadAccess::Type::Destination},
};

bool ParseRoadAccess(string const & roadAccessPath, map<osm::Id, uint32_t> const & osmIdToFeatureId,
                     FeaturesVector const & featuresVector,
                     RoadAccessCollector::RoadAccessByVehicleType & roadAccessByVehicleType)
{
  ifstream stream(roadAccessPath);
  if (!stream)
  {
    LOG(LWARNING, ("Could not open", roadAccessPath));
    return false;
  }

  vector<uint32_t> privateRoads;

  map<uint32_t, RoadAccess::Type> featureType[static_cast<size_t>(VehicleType::Count)];
  map<RoadPoint, RoadAccess::Type> pointType[static_cast<size_t>(VehicleType::Count)];

  auto addFeature = [&](uint32_t featureId, VehicleType vehicleType,
                        RoadAccess::Type roadAccessType, uint64_t osmId) {
    auto & m = featureType[static_cast<size_t>(vehicleType)];
    auto const emplaceRes = m.emplace(featureId, roadAccessType);
    if (!emplaceRes.second && emplaceRes.first->second != roadAccessType)
    {
      LOG(LDEBUG, ("Duplicate road access info for OSM way", osmId, "vehicle:", vehicleType,
                   "access is:", emplaceRes.first->second, "tried:", roadAccessType));
    }
  };

  auto addPoint = [&](RoadPoint const & point, VehicleType vehicleType,
                      RoadAccess::Type roadAccessType) {
    auto & m = pointType[static_cast<size_t>(vehicleType)];
    auto const emplaceRes = m.emplace(point, roadAccessType);
    if (!emplaceRes.second && emplaceRes.first->second != roadAccessType)
    {
      LOG(LDEBUG, ("Duplicate road access info for road point", point, "vehicle:", vehicleType,
                   "access is:", emplaceRes.first->second, "tried:", roadAccessType));
    }
  };

  string line;
  for (uint32_t lineNo = 1;; ++lineNo)
  {
    if (!getline(stream, line))
      break;

    strings::SimpleTokenizer iter(line, kDelim);

    if (!iter)
    {
      LOG(LERROR, ("Error when parsing road access: empty line", lineNo));
      return false;
    }
    VehicleType vehicleType;
    FromString(*iter, vehicleType);
    ++iter;

    if (!iter)
    {
      LOG(LERROR, ("Error when parsing road access: no road access type at line", lineNo, "Line contents:", line));
      return false;
    }
    RoadAccess::Type roadAccessType;
    FromString(*iter, roadAccessType);
    ++iter;

    uint64_t osmId;
    if (!iter || !strings::to_uint64(*iter, osmId))
    {
      LOG(LERROR, ("Error when parsing road access: bad osm id at line", lineNo, "Line contents:", line));
      return false;
    }
    ++iter;

    uint32_t pointIdx;
    if (!iter || !strings::to_uint(*iter, pointIdx))
    {
      LOG(LERROR, ("Error when parsing road access: bad pointIdx at line", lineNo, "Line contents:", line));
      return false;
    }
    ++iter;

    auto const it = osmIdToFeatureId.find(osm::Id::Way(osmId));
    // Even though this osm element has a tag that is interesting for us,
    // we have not created a feature from it. Possible reasons:
    // no primary tag, unsupported type, etc.
    if (it == osmIdToFeatureId.cend())
      continue;

    uint32_t const featureId = it->second;

    if (pointIdx == 0)
      addFeature(featureId, vehicleType, roadAccessType, osmId);
    else
      addPoint(RoadPoint(featureId, pointIdx - 1), vehicleType, roadAccessType);
  }

  for (size_t i = 0; i < static_cast<size_t>(VehicleType::Count); ++i)
    roadAccessByVehicleType[i].SetAccessTypes(move(featureType[i]), move(pointType[i]));

  return true;
}

// If |elem| has access tag from |mapping|, returns corresponding RoadAccess::Type.
// Tags in |mapping| should be mutually exclusive. Caller is responsible for that. If there are
// multiple access tags from |mapping| in |elem|, returns RoadAccess::Type for any of them.
// Returns RoadAccess::Type::Count if |elem| has no access tags from |mapping|.
RoadAccess::Type GetAccessTypeFromMapping(OsmElement const & elem, TagMapping const * mapping)
{
  for (auto const & tag : elem.m_tags)
  {
    auto const it = mapping->find(tag);
    if (it != mapping->cend())
      return it->second;
  }
  return RoadAccess::Type::Count;
}
}  // namespace

namespace routing
{
// RoadAccessTagProcessor --------------------------------------------------------------------------
RoadAccessTagProcessor::RoadAccessTagProcessor(VehicleType vehicleType)
  : m_vehicleType(vehicleType)
{
  switch (vehicleType)
  {
  case VehicleType::Car:
    m_tagMappings.push_back(&kMotorCarTagMapping);
    m_tagMappings.push_back(&kMotorVehicleTagMapping);
    m_tagMappings.push_back(&kVehicleTagMapping);
    m_tagMappings.push_back(&kDefaultTagMapping);
    // Apply barrier tags if we have no {vehicle = ...}, {access = ...} etc.
    m_tagMappings.push_back(&kCarBarriersTagMapping);
    break;
  case VehicleType::Pedestrian:
    m_tagMappings.push_back(&kPedestrianTagMapping);
    m_tagMappings.push_back(&kDefaultTagMapping);
    break;
  case VehicleType::Bicycle:
    m_tagMappings.push_back(&kBicycleTagMapping);
    m_tagMappings.push_back(&kVehicleTagMapping);
    m_tagMappings.push_back(&kDefaultTagMapping);
    // Apply barrier tags if we have no {bicycle = ...}, {access = ...} etc.
    m_tagMappings.push_back(&kBicycleBarriersTagMapping);
    break;
  case VehicleType::Transit:
    // Use kTransitTagMapping to keep transit section empty. We'll use pedestrian section for
    // transit + pedestrian combination.
    m_tagMappings.push_back(&kTransitTagMapping);
    break;
  case VehicleType::Count:
    CHECK(false, ("Bad vehicle type"));
    break;
  }
}

void RoadAccessTagProcessor::Process(OsmElement const & elem, ofstream & oss)
{
  // We will proccess all nodes before ways because of o5m format:
  // all nodes are first, then all ways, then all relations.
  if (elem.type == OsmElement::EntityType::Node)
  {
    RoadAccess::Type accessType = GetAccessType(elem);
    if (accessType != RoadAccess::Type::Yes)
      m_barriers[elem.id] = accessType;
    return;
  }

  if (elem.type != OsmElement::EntityType::Way)
    return;

  // All feature tags.
  auto const accessType = GetAccessType(elem);
  if (accessType != RoadAccess::Type::Yes)
    oss << ToString(m_vehicleType) << " " << ToString(accessType) << " " << elem.id << " "
        << 0 /* wildcard segment Idx */ << endl;

  // Barrier tags.
  for (size_t pointIdx = 0; pointIdx < elem.m_nds.size(); ++pointIdx)
  {
    auto const it = m_barriers.find(elem.m_nds[pointIdx]);
    if (it == m_barriers.cend())
      continue;

    // idx == 0 used as wildcard segment Idx, for nodes we store |pointIdx + 1| instead of |pointIdx|.
    oss << ToString(m_vehicleType) << " " << ToString(it->second) << " " << elem.id << " "
        << pointIdx + 1 << endl;
  }
}

RoadAccess::Type RoadAccessTagProcessor::GetAccessType(OsmElement const & elem) const
{
  for (auto const tagMapping : m_tagMappings)
  {
    auto const accessType = GetAccessTypeFromMapping(elem, tagMapping);
    if (accessType != RoadAccess::Type::Count)
      return accessType;
  }
  return RoadAccess::Type::Yes;
}

// RoadAccessWriter ------------------------------------------------------------
RoadAccessWriter::RoadAccessWriter()
{
  for (size_t i = 0; i < static_cast<size_t>(VehicleType::Count); ++i)
    m_tagProcessors.emplace_back(static_cast<VehicleType>(i));
}

void RoadAccessWriter::Open(string const & filePath)
{
  LOG(LINFO,
      ("Saving information about barriers and road access classes in osm id terms to", filePath));
  m_stream.open(filePath, ofstream::out);

  if (!IsOpened())
    LOG(LINFO, ("Cannot open file", filePath));
}

void RoadAccessWriter::Process(OsmElement const & elem)
{
  if (!IsOpened())
  {
    LOG(LWARNING, ("Tried to write to a closed barriers writer"));
    return;
  }

  for (auto & p : m_tagProcessors)
    p.Process(elem, m_stream);
}

bool RoadAccessWriter::IsOpened() const { return m_stream && m_stream.is_open(); }

// RoadAccessCollector ----------------------------------------------------------
RoadAccessCollector::RoadAccessCollector(string const & dataFilePath, string const & roadAccessPath,
                                         string const & osmIdsToFeatureIdsPath)
{
  map<osm::Id, uint32_t> osmIdToFeatureId;
  if (!ParseOsmIdToFeatureIdMapping(osmIdsToFeatureIdsPath, osmIdToFeatureId))
  {
    LOG(LWARNING, ("An error happened while parsing feature id to osm ids mapping from file:",
                   osmIdsToFeatureIdsPath));
    m_valid = false;
    return;
  }

  FeaturesVectorTest featuresVector(dataFilePath);

  RoadAccessCollector::RoadAccessByVehicleType roadAccessByVehicleType;
  if (!ParseRoadAccess(roadAccessPath, osmIdToFeatureId, featuresVector.GetVector(),
                       roadAccessByVehicleType))
  {
    LOG(LWARNING, ("An error happened while parsing road access from file:", roadAccessPath));
    m_valid = false;
    return;
  }

  m_valid = true;
  m_roadAccessByVehicleType.swap(roadAccessByVehicleType);
}

// Functions ------------------------------------------------------------------
void BuildRoadAccessInfo(string const & dataFilePath, string const & roadAccessPath,
                         string const & osmIdsToFeatureIdsPath)
{
  LOG(LINFO, ("Generating road access info for", dataFilePath));

  RoadAccessCollector collector(dataFilePath, roadAccessPath, osmIdsToFeatureIdsPath);

  if (!collector.IsValid())
  {
    LOG(LWARNING, ("Unable to parse road access in osm terms"));
    return;
  }

  FilesContainerW cont(dataFilePath, FileWriter::OP_WRITE_EXISTING);
  FileWriter writer = cont.GetWriter(ROAD_ACCESS_FILE_TAG);

  RoadAccessSerializer::Serialize(writer, collector.GetRoadAccessAllTypes());
}
}  // namespace routing
