#include "generator/restriction_collector.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include "std/algorithm.hpp"
#include "std/fstream.hpp"

namespace
{
char const kNo[] = "No";
char const kOnly[] = "Only";
char const kDelim[] = ", \t\r\n";

bool ParseLineOfNumbers(strings::SimpleTokenizer & iter, vector<uint64_t> & numbers)
{
  uint64_t number = 0;
  for (; iter; ++iter)
  {
    if (!strings::to_uint64(*iter, number))
      return false;
    numbers.push_back(number);
  }
  return true;
}
}  // namespace

namespace routing
{
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
  LOG(LINFO, ("Number of restrictions: =", m_restrictions.size()));
}

bool RestrictionCollector::IsValid() const
{
  return find_if(begin(m_restrictions), end(m_restrictions),
                 [](Restriction const & r) { return !r.IsValid(); }) == end(m_restrictions);
}

bool RestrictionCollector::ParseFeatureId2OsmIdsMapping(string const & featureId2OsmIdsPath)
{
  ifstream featureId2OsmIdsStream(featureId2OsmIdsPath);
  if (featureId2OsmIdsStream.fail())
    return false;

  string line;
  while (getline(featureId2OsmIdsStream, line))
  {
    vector<uint64_t> osmIds;
    strings::SimpleTokenizer iter(line, kDelim);
    if (!ParseLineOfNumbers(iter, osmIds))
      return false;

    if (osmIds.size() < 2)
      return false;  // Every line should contain at least feature id and osm id.

    uint32_t const featureId = static_cast<uint32_t>(osmIds.front());
    osmIds.erase(osmIds.begin());
    AddFeatureId(featureId, osmIds);
  }
  return true;
}

bool RestrictionCollector::ParseRestrictions(string const & path)
{
  ifstream stream(path);
  if (stream.fail())
    return false;

  string line;
  while (getline(stream, line))
  {
    strings::SimpleTokenizer iter(line, kDelim);
    if (!iter)  // the line is empty
      return false;

    Restriction::Type type;
    if (!FromString(*iter, type))
      return false;

    ++iter;
    vector<uint64_t> osmIds;
    if (!ParseLineOfNumbers(iter, osmIds))
      return false;

    AddRestriction(type, osmIds);
  }
  return true;
}

void RestrictionCollector::ComposeRestrictions()
{
  // Going through all osm ids saved in |m_restrictionIndex| (mentioned in restrictions).
  size_t const numRestrictions = m_restrictions.size();
  for (auto const & osmIdAndIndex : m_restrictionIndex)
  {
    LinkIndex const & index = osmIdAndIndex.second;
    CHECK_LESS(index.m_restrictionNumber, numRestrictions, ());
    Restriction & restriction = m_restrictions[index.m_restrictionNumber];
    CHECK_LESS(index.m_linkNumber, restriction.m_featureIds.size(), ());

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
      continue;  // |osmId| mentioned in restrictions was included in more than one feature.

    uint32_t const & featureId = rangeId.first->second;
    // Adding feature id to restriction coresponded to the osm id.
    restriction.m_featureIds[index.m_linkNumber] = featureId;
  }

  my::SortUnique(m_restrictions);
  // After sorting m_restrictions |m_restrictionIndex| is invalid.
  m_restrictionIndex.clear();
}

void RestrictionCollector::RemoveInvalidRestrictions()
{
  m_restrictions.erase(remove_if(m_restrictions.begin(), m_restrictions.end(),
                                 [](Restriction const & r) { return !r.IsValid(); }),
                       m_restrictions.end());
}

void RestrictionCollector::AddRestriction(Restriction::Type type, vector<uint64_t> const & osmIds)
{
  size_t const numRestrictions = m_restrictions.size();
  m_restrictions.emplace_back(type, osmIds.size());
  for (size_t i = 0; i < osmIds.size(); ++i)
    m_restrictionIndex.emplace_back(osmIds[i], LinkIndex({numRestrictions, i}));
}

void RestrictionCollector::AddFeatureId(uint32_t featureId, vector<uint64_t> const & osmIds)
{
  // Note. One |featureId| could correspond to several osm ids.
  // but for road feature |featureId| corresponds exactly one osm id.
  for (uint64_t const & osmId : osmIds)
    m_osmIds2FeatureId.insert(make_pair(osmId, featureId));
}

string ToString(Restriction::Type const & type)
{
  switch (type)
  {
  case Restriction::Type::No: return kNo;
  case Restriction::Type::Only: return kOnly;
  }
  return "Unknown";
}

bool FromString(string str, Restriction::Type & type)
{
  if (str == kNo)
  {
    type = Restriction::Type::No;
    return true;
  }
  if (str == kOnly)
  {
    type = Restriction::Type::Only;
    return true;
  }

  return false;
}

string DebugPrint(Restriction::Type const & type) { return ToString(type); }

string DebugPrint(RestrictionCollector::LinkIndex const & index)
{
  ostringstream out;
  out << "m_restrictionNumber:" << index.m_restrictionNumber
      << " m_linkNumber:" << index.m_linkNumber << " ";
  return out.str();
}

string DebugPrint(Restriction const & restriction)
{
  ostringstream out;
  out << "m_featureIds:[" << ::DebugPrint(restriction.m_featureIds)
      << "] m_type:" << DebugPrint(restriction.m_type) << " ";
  return out.str();
}
}  // namespace routing
