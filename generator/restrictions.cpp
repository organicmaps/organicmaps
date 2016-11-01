#include "generator/restrictions.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/fstream.hpp"
#include "std/sstream.hpp"

namespace
{
string const kNoStr = "No";
string const kOnlyStr = "Only";

bool ParseLineOfNumbers(istringstream & stream, vector<uint64_t> & numbers)
{
  string numberStr;
  uint64_t number;

  while (stream)
  {
    if (!getline(stream, numberStr, ',' ))
      return true;
    if (numberStr.empty())
      return true;

    if (!strings::to_uint64(numberStr, number))
        return false;

    numbers.push_back(number);
  }
  return true;
}
}  // namespace

namespace routing
{
RestrictionCollector::FeatureId const RestrictionCollector::kInvalidFeatureId =
    numeric_limits<RestrictionCollector::FeatureId>::max();

RestrictionCollector::Restriction::Restriction(Type type, size_t linkNumber) : m_type(type)
{
  m_links.resize(linkNumber, kInvalidFeatureId);
}

RestrictionCollector::Restriction::Restriction(Type type, vector<FeatureId> const & links)
  : m_links(links), m_type(type)
{
}

bool RestrictionCollector::Restriction::IsValid() const
{
  return find(begin(m_links), end(m_links), kInvalidFeatureId) == end(m_links);
}

bool RestrictionCollector::Restriction::operator==(Restriction const & restriction) const
{
  return m_links == restriction.m_links && m_type == restriction.m_type;
}

bool RestrictionCollector::Restriction::operator<(Restriction const & restriction) const
{
  if (m_type != restriction.m_type)
    return m_type < restriction.m_type;

  return m_links < restriction.m_links;
}

RestrictionCollector::RestrictionCollector(string const & restrictionPath,
                                           string const & featureId2OsmIdsPath)
{
  if (!ParseFeatureId2OsmIdsMapping(featureId2OsmIdsPath))
    return;

  if (!ParseRestrictions(restrictionPath))
  {
    m_restrictions.clear();
    return;
  }
  ComposeRestrictions();
  RemoveInvalidRestrictions();
  LOG(LINFO, ("m_restrictions.size() =", m_restrictions.size()));
}

bool RestrictionCollector::IsValid() const
{
  return !m_restrictions.empty()
      && find_if(begin(m_restrictions), end(m_restrictions),
                 [](Restriction const & r){ return !r.IsValid(); }) == end(m_restrictions);
}

bool RestrictionCollector::ParseFeatureId2OsmIdsMapping(string const & featureId2OsmIdsPath)
{
  ifstream featureId2OsmIdsStream(featureId2OsmIdsPath);
  if (featureId2OsmIdsStream.fail())
    return false;

  while (featureId2OsmIdsStream)
  {
    string line;
    if (!getline(featureId2OsmIdsStream, line))
      return true;

    istringstream lineStream(line);
    vector<uint64_t> ids;
    if (!ParseLineOfNumbers(lineStream, ids))
      return false;

    if (ids.size() <= 1)
      return false; // Every line should contain at least feature id and osm id.

    FeatureId const featureId = static_cast<FeatureId>(ids.front());
    ids.erase(ids.begin());
    AddFeatureId(featureId, ids);
  }
  return true;
}

bool RestrictionCollector::ParseRestrictions(string const & restrictionPath)
{
  ifstream restrictionsStream(restrictionPath);
  if (restrictionsStream.fail())
    return false;

  while (restrictionsStream)
  {
    string line;
    if (!getline(restrictionsStream, line))
      return true;
    istringstream lineStream(line);
    string typeStr;
    getline(lineStream, typeStr, ',' );
    Type type;
    if (!FromString(typeStr, type))
      return false;

    vector<uint64_t> osmIds;
    if (!ParseLineOfNumbers(lineStream, osmIds))
      return false;

    AddRestriction(type, osmIds);
  }
  return true;
}

void RestrictionCollector::ComposeRestrictions()
{
  // Going throught all osm id saved in |m_restrictionIndex| (mentioned in restrictions).
  size_t const restrictionSz = m_restrictions.size();
  for (pair<uint64_t, Index> const & osmIdAndIndex : m_restrictionIndex)
  {
    Index const & index = osmIdAndIndex.second;
    CHECK_LESS(index.m_restrictionNumber, restrictionSz, ());
    Restriction & restriction = m_restrictions[index.m_restrictionNumber];
    CHECK_LESS(index.m_linkNumber, restriction.m_links.size(), ());

    uint64_t const & osmId = osmIdAndIndex.first;
    // Checking if there's an osm id belongs to a restriction is saved only once as feature id.
    auto const rangeId = m_osmIds2FeatureId.equal_range(osmId);
    if (rangeId.first == rangeId.second)
    {
      // There's no |osmId| in |m_osmIds2FeatureId| which was mentioned in restrictions.
      // It could happend near mwm border when one of a restriction lines is not included in mwm
      // but the restriction is included.
      continue;
    }
    if (distance(rangeId.first, rangeId.second) != 1)
      continue; // |osmId| mentioned in restrictions was included in more than one feature.

    FeatureId const & featureId = rangeId.first->second;
    // Adding feature id to restriction coresponded to the osm id.
    restriction.m_links[index.m_linkNumber] = featureId;
  }

  my::SortUnique(m_restrictions);
  // After sorting m_restrictions |m_restrictionIndex| is invalid.
  m_restrictionIndex.clear();
}

void RestrictionCollector::RemoveInvalidRestrictions()
{
  m_restrictions.erase(remove_if(m_restrictions.begin(), m_restrictions.end(),
                                 [](Restriction const & r){ return !r.IsValid(); }),
      m_restrictions.end());
}

void RestrictionCollector::AddRestriction(Type type, vector<uint64_t> const & osmIds)
{
  m_restrictions.emplace_back(type, osmIds.size());
  size_t const restrictionCount = m_restrictions.size() - 1;
  for (size_t i = 0; i < osmIds.size(); ++i)
    m_restrictionIndex.emplace_back(osmIds[i], Index({restrictionCount, i}));
}

void RestrictionCollector::AddFeatureId(FeatureId featureId, vector<uint64_t> const & osmIds)
{
  // Note. One |featureId| could correspond to several osm ids.
  // but for road feature |featureId| corresponds exactly one osm id.
  for (uint64_t const & osmId : osmIds)
    m_osmIds2FeatureId.insert(make_pair(osmId, featureId));
}

string ToString(RestrictionCollector::Type const & type)
{
  switch (type)
  {
  case RestrictionCollector::Type::No:
    return kNoStr;
  case RestrictionCollector::Type::Only:
    return kOnlyStr;
  }
  return "Unknown";
}

bool FromString(string str, RestrictionCollector::Type & type)
{
  str.erase(remove_if(str.begin(), str.end(), isspace), str.end());
  if (str == kNoStr)
  {
    type = RestrictionCollector::Type::No;
    return true;
  }
  if (str == kOnlyStr)
  {
    type = RestrictionCollector::Type::Only;
    return true;
  }

  return false;
}

string DebugPrint(RestrictionCollector::Type const & type)
{
  return ToString(type);
}

string DebugPrint(RestrictionCollector::Index const & index)
{
  ostringstream out;
  out << "m_restrictionNumber:" << index.m_restrictionNumber
      << " m_linkNumber:" << index.m_linkNumber << " ";
  return out.str();
}

string DebugPrint(RestrictionCollector::Restriction const & restriction)
{
  ostringstream out;
  out << "m_links:[" << ::DebugPrint(restriction.m_links) << "] m_type:"
      << DebugPrint(restriction.m_type) << " ";
  return out.str();
}
}  // namespace routing
