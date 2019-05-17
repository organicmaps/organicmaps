#include "generator/restriction_collector.hpp"

#include "generator/routing_helpers.hpp"

#include "routing/restriction_loader.hpp"
#include "routing/road_graph.hpp"

#include "routing_common/car_model.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_set>

#include "boost/optional.hpp"

namespace
{
using namespace routing;

char const kNo[] = "No";
char const kOnly[] = "Only";
char const kDelim[] = ", \t\r\n";

bool ParseLineOfWayIds(strings::SimpleTokenizer & iter, std::vector<base::GeoObjectId> & numbers)
{
  uint64_t number = 0;
  for (; iter; ++iter)
  {
    if (!strings::to_uint64(*iter, number))
      return false;
    numbers.push_back(base::MakeOsmWay(number));
  }
  return true;
}
}  // namespace

namespace routing
{
m2::PointD constexpr RestrictionCollector::kNoCoords;

RestrictionCollector::RestrictionCollector(std::string const & osmIdsToFeatureIdPath,
                                           std::unique_ptr<IndexGraph> && graph)
  : m_indexGraph(std::move(graph))
{
  CHECK(ParseRoadsOsmIdToFeatureIdMapping(osmIdsToFeatureIdPath, m_osmIdToFeatureId),
        ("An error happened while parsing feature id to "
         "osm ids mapping from file:", osmIdsToFeatureIdPath));
}

bool RestrictionCollector::Process(std::string const & restrictionPath)
{
  CHECK(m_indexGraph, ());

  SCOPE_GUARD(clean, [this]() {
    m_osmIdToFeatureId.clear();
    m_restrictions.clear();
  });

  if (!ParseRestrictions(restrictionPath))
  {
    LOG(LWARNING, ("An error happened while parsing restrictions from file:",  restrictionPath));
    return false;
  }

  clean.release();

  base::SortUnique(m_restrictions);

  LOG(LDEBUG, ("Number of loaded restrictions:", m_restrictions.size()));
  return true;
}

bool RestrictionCollector::ParseRestrictions(string const & path)
{
  std::ifstream stream(path);
  if (stream.fail())
    return false;

  std::string line;
  while (std::getline(stream, line))
  {
    strings::SimpleTokenizer iter(line, kDelim);
    if (!iter)  // the line is empty
      return false;

    Restriction::Type restrictionType;
    auto viaType = RestrictionWriter::ViaType::Count;
    FromString(*iter, restrictionType);
    ++iter;

    FromString(*iter, viaType);
    ++iter;

    m2::PointD coords = kNoCoords;
    if (viaType == RestrictionWriter::ViaType::Node)
    {
      FromString(*iter, coords.x);
      ++iter;
      FromString(*iter, coords.y);
      ++iter;
    }

    std::vector<base::GeoObjectId> osmIds;
    if (!ParseLineOfWayIds(iter, osmIds))
    {
      LOG(LWARNING, ("Cannot parse osm ids from", path));
      return false;
    }

    if (viaType == RestrictionWriter::ViaType::Node)
      CHECK_EQUAL(osmIds.size(), 2, ("Only |from| and |to| osmId."));

    CHECK_NOT_EQUAL(viaType, RestrictionWriter::ViaType::Count, ());
    AddRestriction(coords, restrictionType, osmIds);
  }
  return true;
}

Joint::Id RestrictionCollector::GetFirstCommonJoint(uint32_t firstFeatureId,
                                                    uint32_t secondFeatureId) const
{
  uint32_t const firstLen = m_indexGraph->GetGeometry().GetRoad(firstFeatureId).GetPointsCount();
  uint32_t const secondLen = m_indexGraph->GetGeometry().GetRoad(secondFeatureId).GetPointsCount();

  auto const firstRoad = m_indexGraph->GetRoad(firstFeatureId);
  auto const secondRoad = m_indexGraph->GetRoad(secondFeatureId);

  std::unordered_set<Joint::Id> used;
  for (uint32_t i = 0; i < firstLen; ++i)
  {
    if (firstRoad.GetJointId(i) != Joint::kInvalidId)
      used.emplace(firstRoad.GetJointId(i));
  }

  for (uint32_t i = 0; i < secondLen; ++i)
  {
    if (used.count(secondRoad.GetJointId(i)) != 0)
      return secondRoad.GetJointId(i);
  }

  return Joint::kInvalidId;
}

bool RestrictionCollector::FeatureHasPointWithCoords(uint32_t featureId,
                                                     m2::PointD const & coords) const
{
  CHECK(m_indexGraph, ());
  auto & roadGeometry = m_indexGraph->GetGeometry().GetRoad(featureId);
  uint32_t const pointsCount = roadGeometry.GetPointsCount();
  for (uint32_t i = 0; i < pointsCount; ++i)
  {
    if (base::AlmostEqualAbs(roadGeometry.GetPoint(i), coords, kPointsEqualEpsilon))
      return true;
  }

  return false;
}

bool RestrictionCollector::FeaturesAreCross(m2::PointD const & coords,
                                            uint32_t prev, uint32_t cur) const
{
  if (coords == kNoCoords)
    return GetFirstCommonJoint(prev, cur) != Joint::kInvalidId;

  return FeatureHasPointWithCoords(prev, coords) && FeatureHasPointWithCoords(cur, coords);
}

bool RestrictionCollector::IsRestrictionValid(m2::PointD const & coords, 
                                              std::vector<uint32_t> const & featureIds) const
{
  if (featureIds.empty() || !m_indexGraph->IsRoad(featureIds[0]))
    return false;

  for (size_t i = 1; i < featureIds.size(); ++i)
  {
    auto prev = featureIds[i - 1];
    auto cur = featureIds[i];
    if (!m_indexGraph->IsRoad(cur))
      return false;

    if (!FeaturesAreCross(coords, prev, cur))
      return false;
  }

  return true;
}

bool RestrictionCollector::AddRestriction(m2::PointD const & coords,
                                          Restriction::Type restrictionType,
                                          std::vector<base::GeoObjectId> const & osmIds)
{
  std::vector<uint32_t> featureIds(osmIds.size());
  for (size_t i = 0; i < osmIds.size(); ++i)
  {
    auto const result = m_osmIdToFeatureId.find(osmIds[i]);
    if (result == m_osmIdToFeatureId.cend())
    {
      // It could happend near mwm border when one of a restriction lines is not included in mwm
      // but the restriction is included.
      return false;
    }

    // Only one feature id is found for |osmIds[i]|.
    featureIds[i] = result->second;
  }

  if (!IsRestrictionValid(coords, featureIds))
    return false;

  m_restrictions.emplace_back(restrictionType, featureIds);
  return true;
}

void RestrictionCollector::AddFeatureId(uint32_t featureId, base::GeoObjectId osmId)
{
  ::routing::AddFeatureId(osmId, featureId, m_osmIdToFeatureId);
}

void FromString(std::string const & str, Restriction::Type & type)
{
  if (str == kNo)
  {
    type = Restriction::Type::No;
    return;
  }

  if (str == kOnly)
  {
    type = Restriction::Type::Only;
    return;
  }

  CHECK(false, ("Invalid line:", str, "expected:", kNo, "or", kOnly));
  UNREACHABLE();
}

void FromString(std::string const & str, RestrictionWriter::ViaType & type)
{
  if (str == RestrictionWriter::kNodeString)
  {
    type = RestrictionWriter::ViaType::Node;
    return;
  }

  if (str == RestrictionWriter::kWayString)
  {
    type = RestrictionWriter::ViaType::Way;
    return;
  }

  CHECK(false, ("Invalid line:", str, "expected:", RestrictionWriter::kNodeString,
                "or", RestrictionWriter::kWayString));
}

void FromString(std::string const & str, double & number)
{
  CHECK(strings::to_double(str.c_str(), number), ());
}
}  // namespace routing
