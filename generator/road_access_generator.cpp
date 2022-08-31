#include "generator/road_access_generator.hpp"

#include "generator/feature_builder.hpp"
#include "generator/final_processor_utils.hpp"
#include "generator/routing_helpers.hpp"

#include "routing/road_access.hpp"
#include "routing/road_access_serialization.hpp"
#include "routing/routing_helpers.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/features_vector.hpp"

#include "coding/file_writer.hpp"
#include "coding/files_container.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"
#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include <algorithm>
#include <initializer_list>
#include <limits>
#include <unordered_map>
#include <utility>

#include "3party/opening_hours/opening_hours.hpp"

namespace routing_builder
{
using namespace feature;
using namespace routing;
using namespace generator;
using namespace std;

using TagMapping = RoadAccessTagProcessor::TagMapping;
using ConditionalTagsList = RoadAccessTagProcessor::ConditionalTagsList;

// Get from: https://taginfo.openstreetmap.org/search?q=%3Aconditional
// @{
vector<string> const kCarAccessConditionalTags = {
    "motor_vehicle:conditional", "motorcar:conditional", "vehicle:conditional",
};

vector<string> const kDefaultAccessConditionalTags = {
    "access:conditional"
};

vector<string> const kPedestrianAccessConditionalTags = {
    "foot:conditional"
};

vector<string> const kBycicleAccessConditionalTags = {
    "bicycle:conditional"
};
// @}

// Some tags assume access:conditional in fact, but doesn't have it.
// For example if road is tagged as "winter_road = yes",
// for routing it is like: "access:conditional = no @ (Feb - Dec)"
map<OsmElement::Tag, string> kTagToAccessConditional = {
    {OsmElement::Tag("winter_road", "yes"), "no @ (Mar - Nov)"},
    {OsmElement::Tag("ice_road", "yes"), "no @ (Mar - Nov)"}
};

TagMapping const kMotorCarTagMapping = {
    {OsmElement::Tag("motorcar", "yes"), RoadAccess::Type::Yes},
    {OsmElement::Tag("motorcar", "designated"), RoadAccess::Type::Yes},
    {OsmElement::Tag("motorcar", "permissive"), RoadAccess::Type::Yes},
    {OsmElement::Tag("motorcar", "no"), RoadAccess::Type::No},
    {OsmElement::Tag("motorcar", "private"), RoadAccess::Type::Private},
    {OsmElement::Tag("motorcar", "destination"), RoadAccess::Type::Destination},
};

TagMapping const kMotorVehicleTagMapping = {
    {OsmElement::Tag("motor_vehicle", "yes"), RoadAccess::Type::Yes},
    {OsmElement::Tag("motor_vehicle", "designated"), RoadAccess::Type::Yes},
    {OsmElement::Tag("motor_vehicle", "permissive"), RoadAccess::Type::Yes},
    {OsmElement::Tag("motor_vehicle", "no"), RoadAccess::Type::No},
    {OsmElement::Tag("motor_vehicle", "private"), RoadAccess::Type::Private},
    {OsmElement::Tag("motor_vehicle", "destination"), RoadAccess::Type::Destination},
};

TagMapping const kVehicleTagMapping = {
    {OsmElement::Tag("vehicle", "yes"), RoadAccess::Type::Yes},
    {OsmElement::Tag("vehicle", "designated"), RoadAccess::Type::Yes},
    {OsmElement::Tag("vehicle", "permissive"), RoadAccess::Type::Yes},
    {OsmElement::Tag("vehicle", "no"), RoadAccess::Type::No},
    {OsmElement::Tag("vehicle", "private"), RoadAccess::Type::Private},
    {OsmElement::Tag("vehicle", "destination"), RoadAccess::Type::Destination},
};

TagMapping const kCarBarriersTagMapping = {
    {OsmElement::Tag("barrier", "block"), RoadAccess::Type::No},
    {OsmElement::Tag("barrier", "bollard"), RoadAccess::Type::No},
    {OsmElement::Tag("barrier", "cycle_barrier"), RoadAccess::Type::No},
    {OsmElement::Tag("barrier", "chain"), RoadAccess::Type::Private},
    {OsmElement::Tag("barrier", "gate"), RoadAccess::Type::Private},
    {OsmElement::Tag("barrier", "lift_gate"), RoadAccess::Type::Private},
    {OsmElement::Tag("barrier", "swing_gate"), RoadAccess::Type::Private},

    // TODO (@gmoryes) The types below should be added.
    //  {OsmElement::Tag("barrier", "log"), RoadAccess::Type::No},
    //  {OsmElement::Tag("barrier", "motorcycle_barrier"), RoadAccess::Type::No},
};

TagMapping const kPedestrianTagMapping = {
    {OsmElement::Tag("foot", "yes"), RoadAccess::Type::Yes},
    {OsmElement::Tag("foot", "designated"), RoadAccess::Type::Yes},
    {OsmElement::Tag("foot", "permissive"), RoadAccess::Type::Yes},
    {OsmElement::Tag("foot", "no"), RoadAccess::Type::No},
    {OsmElement::Tag("foot", "private"), RoadAccess::Type::Private},
    {OsmElement::Tag("foot", "destination"), RoadAccess::Type::Destination},
};

TagMapping const kBicycleTagMapping = {
    {OsmElement::Tag("bicycle", "yes"), RoadAccess::Type::Yes},
    {OsmElement::Tag("bicycle", "designated"), RoadAccess::Type::Yes},
    {OsmElement::Tag("bicycle", "permissive"), RoadAccess::Type::Yes},
    {OsmElement::Tag("bicycle", "no"), RoadAccess::Type::No},
    {OsmElement::Tag("bicycle", "private"), RoadAccess::Type::Private},
    {OsmElement::Tag("bicycle", "destination"), RoadAccess::Type::Destination},
};

TagMapping const kBicycleBarriersTagMapping = {
    {OsmElement::Tag("barrier", "cycle_barrier"), RoadAccess::Type::No},
    {OsmElement::Tag("barrier", "gate"), RoadAccess::Type::Private},
    // TODO (@gmoryes) The types below should be added.
    //  {OsmElement::Tag("barrier", "kissing_gate"), RoadAccess::Type::Private},
};

// Allow everything to keep transit section empty. We'll use pedestrian section for
// transit + pedestrian combination.
// Empty mapping leads to default RoadAccess::Type::Yes access type for all roads.
TagMapping const kTransitTagMapping = {};

TagMapping const kDefaultTagMapping = {
    {OsmElement::Tag("access", "yes"), RoadAccess::Type::Yes},
    {OsmElement::Tag("access", "permissive"), RoadAccess::Type::Yes},
    {OsmElement::Tag("access", "no"), RoadAccess::Type::No},
    {OsmElement::Tag("access", "private"), RoadAccess::Type::Private},
    {OsmElement::Tag("access", "destination"), RoadAccess::Type::Destination},
};

set<OsmElement::Tag> const kHighwaysWhereIgnoreBarriersWithoutAccess = {
    {OsmElement::Tag("highway", "motorway")},
    {OsmElement::Tag("highway", "motorway_link")},
    {OsmElement::Tag("highway", "primary")},
    {OsmElement::Tag("highway", "primary_link")},
    {OsmElement::Tag("highway", "secondary")},
    {OsmElement::Tag("highway", "secondary_link")},
    {OsmElement::Tag("highway", "tertiary")},
    {OsmElement::Tag("highway", "tertiary_link")},
    {OsmElement::Tag("highway", "trunk")},
    {OsmElement::Tag("highway", "trunk_link")}
};

bool ParseRoadAccess(string const & roadAccessPath, OsmIdToFeatureIds const & osmIdToFeatureIds,
                     RoadAccessCollector::RoadAccessByVehicleType & roadAccessByVehicleType)
{
  ifstream stream(roadAccessPath);
  if (!stream)
  {
    LOG(LWARNING, ("Could not open", roadAccessPath));
    return false;
  }

  RoadAccess::WayToAccess featureType[static_cast<size_t>(VehicleType::Count)];
  RoadAccess::PointToAccess pointType[static_cast<size_t>(VehicleType::Count)];

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

    strings::SimpleTokenizer iter(line, " \t\r\n");

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
    if (!iter || !strings::to_uint(*iter, osmId))
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

    auto const it = osmIdToFeatureIds.find(base::MakeOsmWay(osmId));
    // Even though this osm element has a tag that is interesting for us,
    // we have not created a feature from it. Possible reasons:
    // no primary tag, unsupported type, etc.
    if (it == osmIdToFeatureIds.cend())
      continue;

    // @TODO(bykoianko) All the feature ids should be used instead of |it->second.front()|.
    CHECK(!it->second.empty(), ());
    uint32_t const featureId = it->second.front();

    if (pointIdx == 0)
      addFeature(featureId, vehicleType, roadAccessType, osmId);
    else
      addPoint(RoadPoint(featureId, pointIdx - 1), vehicleType, roadAccessType);
  }

  for (size_t i = 0; i < static_cast<size_t>(VehicleType::Count); ++i)
    roadAccessByVehicleType[i].SetAccess(move(featureType[i]), move(pointType[i]));

  return true;
}

void ParseRoadAccessConditional(
    string const & roadAccessPath, OsmIdToFeatureIds const & osmIdToFeatureIds,
    RoadAccessCollector::RoadAccessByVehicleType & roadAccessByVehicleType)
{
  ifstream stream(roadAccessPath);
  if (!stream)
  {
    LOG(LWARNING, ("Could not open", roadAccessPath));
    return;
  }

  size_t constexpr kVehicleCount = static_cast<size_t>(VehicleType::Count);
  CHECK_EQUAL(kVehicleCount, roadAccessByVehicleType.size(), ());
  array<RoadAccess::WayToAccessConditional, kVehicleCount> wayToAccessConditional;
  // TODO point is not supported yet.
  array<RoadAccess::PointToAccessConditional, kVehicleCount> pointToAccessConditional;

  string line;
  VehicleType vehicleType = VehicleType::Count;
  while (getline(stream, line))
  {
    strings::TokenizeIterator<strings::SimpleDelimiter, std::string::const_iterator, true /* KeepEmptyTokens */>
        strIt(line.begin(), line.end(), strings::SimpleDelimiter('\t'));

    CHECK(strIt, (line));
    string_view buffer = *strIt;
    strings::Trim(buffer);
    FromString(buffer, vehicleType);
    CHECK_NOT_EQUAL(vehicleType, VehicleType::Count, (line, buffer));

    auto const moveIterAndCheck = [&]()
    {
      ++strIt;
      CHECK(strIt, (line));
      return *strIt;
    };

    uint64_t osmId = 0;
    buffer = moveIterAndCheck();
    strings::Trim(buffer);
    CHECK(strings::to_uint(buffer, osmId), (line, buffer));

    size_t accessNumber = 0;
    buffer = moveIterAndCheck();
    strings::Trim(buffer);
    CHECK(strings::to_uint(buffer, accessNumber), (line, buffer));
    CHECK_NOT_EQUAL(accessNumber, 0, (line));

    RoadAccess::Conditional conditional;
    for (size_t i = 0; i < accessNumber; ++i)
    {
      buffer = moveIterAndCheck();
      strings::Trim(buffer);
      RoadAccess::Type roadAccessType = RoadAccess::Type::Count;
      FromString(buffer, roadAccessType);
      CHECK_NOT_EQUAL(roadAccessType, RoadAccess::Type::Count, (line));

      /// @todo Avoid temporary string when OpeningHours (boost::spirit) will allow string_view.
      string strBuffer(moveIterAndCheck());
      strings::Trim(strBuffer);
      osmoh::OpeningHours openingHours(strBuffer);
      if (!openingHours.IsValid())
        continue;

      conditional.Insert(roadAccessType, move(openingHours));
    }

    if (conditional.IsEmpty())
      continue;

    auto const it = osmIdToFeatureIds.find(base::MakeOsmWay(osmId));
    if (it == osmIdToFeatureIds.end())
      continue;
    // @TODO(bykoianko) All the feature ids should be used instead of |it->second.front()|.
    CHECK(!osmIdToFeatureIds.empty(), ());
    uint32_t const featureId = it->second.front();

    wayToAccessConditional[static_cast<size_t>(vehicleType)].emplace(featureId, move(conditional));
  }

  for (size_t i = 0; i < roadAccessByVehicleType.size(); ++i)
  {
    roadAccessByVehicleType[i].SetAccessConditional(move(wayToAccessConditional[i]),
                                                    move(pointToAccessConditional[i]));
  }
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

optional<pair<string, string>> GetTagValueConditionalAccess(
    OsmElement const & elem, vector<ConditionalTagsList> const & tagsList)
{
  if (tagsList.empty())
    return {};

  for (auto const & tags : tagsList)
  {
    for (auto const & tag : tags)
    {
      if (elem.HasTag(tag))
        return make_pair(tag, elem.GetTag(tag));
    }
  }

  for (auto const & [tag, access] : kTagToAccessConditional)
  {
    if (elem.HasTag(tag.m_key, tag.m_value))
    {
      CHECK(!tagsList.back().empty(), ());
      auto const anyAccessConditionalTag = tagsList.back().back();
      return make_pair(anyAccessConditionalTag, access);
    }
  }

  return {};
}

// "motor_vehicle:conditional" -> "motor_vehicle"
// "access:conditional" -> "access"
// etc.
string GetVehicleTypeForAccessConditional(string const & accessConditionalTag)
{
  auto const pos = accessConditionalTag.find(":");
  CHECK_NOT_EQUAL(pos, string::npos, (accessConditionalTag));

  string result(accessConditionalTag.begin(), accessConditionalTag.begin() + pos);
  return result;
}

// RoadAccessTagProcessor --------------------------------------------------------------------------
RoadAccessTagProcessor::RoadAccessTagProcessor(VehicleType vehicleType)
    : m_vehicleType(vehicleType)
{
  switch (vehicleType)
  {
  case VehicleType::Car:
    m_accessMappings.push_back(&kMotorCarTagMapping);
    m_accessMappings.push_back(&kMotorVehicleTagMapping);
    m_accessMappings.push_back(&kVehicleTagMapping);
    m_accessMappings.push_back(&kDefaultTagMapping);
    m_barrierMappings.push_back(&kCarBarriersTagMapping);
    m_hwIgnoreBarriersWithoutAccess = kHighwaysWhereIgnoreBarriersWithoutAccess;
    m_conditionalTagsVector.push_back(kCarAccessConditionalTags);
    m_conditionalTagsVector.push_back(kDefaultAccessConditionalTags);
    break;
  case VehicleType::Pedestrian:
    m_accessMappings.push_back(&kPedestrianTagMapping);
    m_accessMappings.push_back(&kDefaultTagMapping);
    m_conditionalTagsVector.push_back(kPedestrianAccessConditionalTags);
    m_conditionalTagsVector.push_back(kDefaultAccessConditionalTags);
    break;
  case VehicleType::Bicycle:
    m_accessMappings.push_back(&kBicycleTagMapping);
    m_accessMappings.push_back(&kVehicleTagMapping);
    m_accessMappings.push_back(&kDefaultTagMapping);
    m_barrierMappings.push_back(&kBicycleBarriersTagMapping);
    m_hwIgnoreBarriersWithoutAccess = kHighwaysWhereIgnoreBarriersWithoutAccess;
    m_conditionalTagsVector.push_back(kBycicleAccessConditionalTags);
    m_conditionalTagsVector.push_back(kDefaultAccessConditionalTags);
    break;
  case VehicleType::Transit:
    // Use kTransitTagMapping to keep transit section empty. We'll use pedestrian section for
    // transit + pedestrian combination.
    m_accessMappings.push_back(&kTransitTagMapping);
    break;
  case VehicleType::Count:
    CHECK(false, ("Bad vehicle type"));
    break;
  }
}

void RoadAccessTagProcessor::Process(OsmElement const & elem)
{
  auto const getAccessType = [&](vector<TagMapping const *> const & mapping)
  {
    for (auto const tagMapping : mapping)
    {
      auto const accessType = GetAccessTypeFromMapping(elem, tagMapping);
      if (accessType != RoadAccess::Type::Count)
        return optional<RoadAccess::Type>(accessType);
    }

    return optional<RoadAccess::Type>();
  };

  if (auto op = getAccessType(m_accessMappings))
  {
    if (*op == RoadAccess::Type::Yes)
      return;

    switch (elem.m_type)
    {
    case OsmElement::EntityType::Node: m_barriersWithAccessTag.emplace(elem.m_id, *op); return;
    case OsmElement::EntityType::Way: m_wayToAccess.emplace(elem.m_id, *op); return;
    default: return;
    }
  }

  if (!elem.IsNode())
    return;

  // Apply barrier tags if we have no {vehicle = ...}, {access = ...} etc.
  if (auto op = getAccessType(m_barrierMappings))
    m_barriersWithoutAccessTag.emplace(elem.m_id, *op);
}

void RoadAccessTagProcessor::ProcessConditional(OsmElement const & elem)
{
  if (!elem.IsWay())
    return;

  auto op = GetTagValueConditionalAccess(elem, m_conditionalTagsVector);
  if (!op)
    return;

  auto const & [tag, value] = *op;
  auto const & parser = AccessConditionalTagParser::Instance();
  auto accesses = parser.ParseAccessConditionalTag(tag, value);
  for (auto & access : accesses)
  {
    if (access.m_accessType != RoadAccess::Type::Count)
      m_wayToAccessConditional[elem.m_id].emplace_back(move(access));
  }
}

void RoadAccessTagProcessor::WriteWayToAccess(ostream & stream)
{
  // All feature tags.
  for (auto const & i : m_wayToAccess)
  {
    stream << ToString(m_vehicleType) << " " << ToString(i.second) << " " << i.first << " "
           << 0 /* wildcard segment Idx */ << endl;
  }
}

void RoadAccessTagProcessor::WriteWayToAccessConditional(std::ostream & stream)
{
  for (auto const & [osmId, accesses] : m_wayToAccessConditional)
  {
    CHECK(!accesses.empty(), ());
    stream << ToString(m_vehicleType) << '\t' << osmId << '\t' << accesses.size() << '\t';
    for (auto const & access : accesses)
    {
      string oh;
      replace_copy(cbegin(access.m_openingHours), cend(access.m_openingHours), back_inserter(oh), '\t', ' ');
      stream << ToString(access.m_accessType) << '\t' << oh << '\t';
    }
    stream << endl;
  }
}

void RoadAccessTagProcessor::WriteBarrierTags(ostream & stream, uint64_t id,
                                              vector<uint64_t> const & points,
                                              bool ignoreBarrierWithoutAccessOnThisWay)
{
  for (size_t pointIdx = 0; pointIdx < points.size(); ++pointIdx)
  {
    auto it = m_barriersWithAccessTag.find(points[pointIdx]);
    if (it == m_barriersWithAccessTag.cend())
    {
      it = m_barriersWithoutAccessTag.find(points[pointIdx]);
      if (it == m_barriersWithoutAccessTag.cend())
        continue;

      if (m_hwIgnoreBarriersWithoutAccess && ignoreBarrierWithoutAccessOnThisWay)
        continue;
    }

    RoadAccess::Type const roadAccessType = it->second;
    // idx == 0 used as wildcard segment Idx, for nodes we store |pointIdx + 1| instead of |pointIdx|.
    stream << ToString(m_vehicleType) << " " << ToString(roadAccessType) << " " << id << " "
           << pointIdx + 1 << endl;
  }
}

void RoadAccessTagProcessor::Merge(RoadAccessTagProcessor const & other)
{
  CHECK_EQUAL(m_vehicleType, other.m_vehicleType, ());

  m_barriersWithAccessTag.insert(begin(other.m_barriersWithAccessTag),
                                 end(other.m_barriersWithAccessTag));
  m_barriersWithoutAccessTag.insert(begin(other.m_barriersWithoutAccessTag),
                                    end(other.m_barriersWithoutAccessTag));
  m_wayToAccess.insert(begin(other.m_wayToAccess), end(other.m_wayToAccess));
  m_wayToAccessConditional.insert(begin(other.m_wayToAccessConditional),
                                  end(other.m_wayToAccessConditional));
}

// RoadAccessWriter ------------------------------------------------------------
RoadAccessWriter::RoadAccessWriter(string const & filename)
    : generator::CollectorInterface(filename)
    , m_waysFilename(GetTmpFilename() + ".roads_ids")
    , m_waysWriter(make_unique<FileWriter>(m_waysFilename))
{
  for (size_t i = 0; i < static_cast<size_t>(VehicleType::Count); ++i)
    m_tagProcessors.emplace_back(static_cast<VehicleType>(i));
}

RoadAccessWriter::~RoadAccessWriter()
{
  CHECK(Platform::RemoveFileIfExists(m_waysFilename), ());
}

shared_ptr<generator::CollectorInterface> RoadAccessWriter::Clone(
    shared_ptr<generator::cache::IntermediateDataReaderInterface> const &) const
{
  return make_shared<RoadAccessWriter>(GetFilename());
}

void RoadAccessWriter::CollectFeature(FeatureBuilder const & fb, OsmElement const & elem)
{
  for (auto & p : m_tagProcessors)
  {
    p.Process(elem);
    p.ProcessConditional(elem);
  }

  if (!IsRoad(fb.GetTypes()))
    return;

  uint8_t ignoreBarrierWithoutAccessOnThisWay = 0;
  for (auto const & tag : elem.m_tags)
  {
    if (kHighwaysWhereIgnoreBarriersWithoutAccess.count(tag))
    {
      ignoreBarrierWithoutAccessOnThisWay = 1;
      break;
    }
  }

  WriteToSink(*m_waysWriter, elem.m_id);
  WriteToSink(*m_waysWriter, ignoreBarrierWithoutAccessOnThisWay);
  rw::WriteVectorOfPOD(*m_waysWriter, elem.m_nodes);
}

void RoadAccessWriter::Finish()
{
  m_waysWriter.reset({});
}

void RoadAccessWriter::Save()
{
  CHECK(!m_waysWriter, ("Finish() has not been called."));
  ofstream out;
  out.exceptions(fstream::failbit | fstream::badbit);
  out.open(GetFilename());

  for (auto & p : m_tagProcessors)
    p.WriteWayToAccess(out);

  FileReader reader(m_waysFilename);
  ReaderSource<FileReader> src(reader);
  while (src.Size() > 0)
  {
    auto const wayId = ReadPrimitiveFromSource<uint64_t>(src);

    auto const ignoreBarrierWithoutAccessOnThisWay =
        static_cast<bool>(ReadPrimitiveFromSource<uint8_t>(src));

    vector<uint64_t> nodes;
    rw::ReadVectorOfPOD(src, nodes);
    for (auto & p : m_tagProcessors)
      p.WriteBarrierTags(out, wayId, nodes, ignoreBarrierWithoutAccessOnThisWay);
  }

  ofstream outConditional;
  outConditional.exceptions(fstream::failbit | fstream::badbit);
  outConditional.open(GetFilename() + ROAD_ACCESS_CONDITIONAL_EXT);
  for (auto & p : m_tagProcessors)
    p.WriteWayToAccessConditional(outConditional);
}

void RoadAccessWriter::OrderCollectedData()
{
  for (auto const & filename : {GetFilename(), GetFilename() + ROAD_ACCESS_CONDITIONAL_EXT})
    OrderTextFileByLine(filename);
}

void RoadAccessWriter::Merge(generator::CollectorInterface const & collector)
{
  collector.MergeInto(*this);
}

void RoadAccessWriter::MergeInto(RoadAccessWriter & collector) const
{
  auto & otherProcessors = collector.m_tagProcessors;
  CHECK_EQUAL(m_tagProcessors.size(), otherProcessors.size(), ());

  for (size_t i = 0; i < otherProcessors.size(); ++i)
    otherProcessors[i].Merge(m_tagProcessors[i]);

  CHECK(!m_waysWriter || !collector.m_waysWriter, ("Finish() has not been called."));
  base::AppendFileToFile(m_waysFilename, collector.m_waysFilename);
}

// RoadAccessCollector ----------------------------------------------------------
RoadAccessCollector::RoadAccessCollector(string const & dataFilePath, string const & roadAccessPath,
                                         string const & osmIdsToFeatureIdsPath)
{
  OsmIdToFeatureIds osmIdToFeatureIds;
  if (!ParseWaysOsmIdToFeatureIdMapping(osmIdsToFeatureIdsPath, osmIdToFeatureIds))
  {
    LOG(LWARNING, ("An error happened while parsing feature id to osm ids mapping from file:",
                   osmIdsToFeatureIdsPath));
    m_valid = false;
    return;
  }

  RoadAccessCollector::RoadAccessByVehicleType roadAccessByVehicleType;
  if (!ParseRoadAccess(roadAccessPath, osmIdToFeatureIds, roadAccessByVehicleType))
  {
    LOG(LWARNING, ("An error happened while parsing road access from file:", roadAccessPath));
    m_valid = false;
    return;
  }

  auto const roadAccessConditional = roadAccessPath + ROAD_ACCESS_CONDITIONAL_EXT;
  ParseRoadAccessConditional(roadAccessConditional, osmIdToFeatureIds, roadAccessByVehicleType);

  m_valid = true;
  m_roadAccessByVehicleType.swap(roadAccessByVehicleType);
}

// AccessConditionalTagParser ----------------------------------------------------------------------
// static
AccessConditionalTagParser const & AccessConditionalTagParser::Instance()
{
  static AccessConditionalTagParser instance;
  return instance;
}

AccessConditionalTagParser::AccessConditionalTagParser()
{
  m_vehiclesToRoadAccess.push_back(kMotorCarTagMapping);
  m_vehiclesToRoadAccess.push_back(kMotorVehicleTagMapping);
  m_vehiclesToRoadAccess.push_back(kVehicleTagMapping);
  m_vehiclesToRoadAccess.push_back(kPedestrianTagMapping);
  m_vehiclesToRoadAccess.push_back(kBicycleTagMapping);
  m_vehiclesToRoadAccess.push_back(kDefaultTagMapping);
}

vector<AccessConditional> AccessConditionalTagParser::ParseAccessConditionalTag(
    string const & tag, string const & value) const
{
  size_t pos = 0;

  string const vehicleType = GetVehicleTypeForAccessConditional(tag);
  vector<AccessConditional> accessConditionals;
  while (pos < value.size())
  {
    AccessConditional access;
    auto accessOp = ReadUntilSymbol(value, pos, '@');
    if (!accessOp)
      break;

    string accessString;
    tie(pos, accessString) = *accessOp;
    ++pos;  // skip '@'
    strings::Trim(accessString);

    access.m_accessType = GetAccessByVehicleAndStringValue(vehicleType, accessString);

    auto substrOp = ReadUntilSymbol(value, pos, ';');
    if (!substrOp)
    {
      auto openingHours = std::string(value.begin() + pos, value.end());
      access.m_openingHours = TrimAndDropAroundParentheses(move(openingHours));
      pos = value.size();
    }
    else
    {
      auto [newPos, _] = *substrOp;
      // We cannot distinguish these two situations:
      //   1) no @ Mo-Fr ; yes @ Sa-Su
      //   2) no @ Mo-Fr ; Sa 10:00-19:00
      auto nextAccessOp = ReadUntilSymbol(value, newPos, '@');
      if (nextAccessOp)
      {
        // This is 1) case.
        auto openingHours = std::string(value.begin() + pos, value.begin() + newPos);
        access.m_openingHours = TrimAndDropAroundParentheses(move(openingHours));
        pos = newPos;
        ++pos;  // skip ';'
      }
      else
      {
        // This is 2) case.
        auto openingHours = std::string(value.begin() + pos, value.end());
        access.m_openingHours = TrimAndDropAroundParentheses(move(openingHours));
        pos = value.size();
      }
    }

    accessConditionals.emplace_back(move(access));
  }

  return accessConditionals;
}

optional<pair<size_t, string>> AccessConditionalTagParser::ReadUntilSymbol(string const & input,
                                                                           size_t startPos,
                                                                           char symbol) const
{
  string result;
  while (startPos < input.size() && input[startPos] != symbol)
  {
    result += input[startPos];
    ++startPos;
  }

  if (input[startPos] == symbol)
    return make_pair(startPos, result);

  return nullopt;
}

RoadAccess::Type AccessConditionalTagParser::GetAccessByVehicleAndStringValue(
    string const & vehicleFromTag, string const & stringAccessValue) const
{
  for (auto const & vehicleToAccess : m_vehiclesToRoadAccess)
  {
    auto const it = vehicleToAccess.find({vehicleFromTag, stringAccessValue});
    if (it != vehicleToAccess.end())
      return it->second;
  }

  return RoadAccess::Type::Count;
}

string AccessConditionalTagParser::TrimAndDropAroundParentheses(string input) const
{
  strings::Trim(input);

  if (!input.empty() && input.back() == ';')
    input.pop_back();

  if (input.size() >= 2 && input.front() == '(' && input.back() == ')')
  {
    input.erase(input.begin());
    input.pop_back();
  }

  return input;
}

// Functions ------------------------------------------------------------------
bool BuildRoadAccessInfo(string const & dataFilePath, string const & roadAccessPath,
                         string const & osmIdsToFeatureIdsPath)
{
  LOG(LINFO, ("Generating road access info for", dataFilePath));

  RoadAccessCollector collector(dataFilePath, roadAccessPath, osmIdsToFeatureIdsPath);

  if (!collector.IsValid())
  {
    LOG(LWARNING, ("Unable to parse road access in osm terms"));
    return false;
  }

  FilesContainerW cont(dataFilePath, FileWriter::OP_WRITE_EXISTING);
  auto writer = cont.GetWriter(ROAD_ACCESS_FILE_TAG);

  RoadAccessSerializer::Serialize(*writer, collector.GetRoadAccessAllTypes());
  return true;
}
}  // namespace routing_builder
