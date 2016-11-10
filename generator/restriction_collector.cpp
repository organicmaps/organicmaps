#include "generator/restriction_collector.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
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
                                           string const & featureIdToOsmIdsPath)
{
  MY_SCOPE_GUARD(parseErrorGuard, [this]{
    m_osmIdToFeatureId.clear();
    m_restrictions.clear();
  });

  if (!ParseFeatureId2OsmIdsMapping(featureIdToOsmIdsPath)
      || !ParseRestrictions(restrictionPath))
  {
    LOG(LWARNING, ("An error happened while parsing", featureIdToOsmIdsPath, "or", restrictionPath));
    return;
  }
  parseErrorGuard.release();

  my::SortUnique(m_restrictions);

  if (!IsValid())
    LOG(LERROR, ("Some restrictions are not valid."));
  LOG(LDEBUG, ("Number of loaded restrictions:", m_restrictions.size()));
}

bool RestrictionCollector::IsValid() const
{
  return find_if(begin(m_restrictions), end(m_restrictions),
                 [](Restriction const & r) { return !r.IsValid(); }) == end(m_restrictions);
}

bool RestrictionCollector::ParseFeatureId2OsmIdsMapping(string const & featureIdToOsmIdsPath)
{
  ifstream featureId2OsmIdsStream(featureIdToOsmIdsPath);
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
      return false;  // Every line should contain feature id and at least one osm id.

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
    {
      LOG(LWARNING, ("Cannot parse a restriction type", *iter, "form", path));
      return false;
    }

    ++iter;
    vector<uint64_t> osmIds;
    if (!ParseLineOfNumbers(iter, osmIds))
    {
      LOG(LWARNING, ("Cannot parse osm ids from", path));
      return false;
    }

    AddRestriction(type, osmIds);
  }
  return true;
}

bool RestrictionCollector::AddRestriction(Restriction::Type type, vector<uint64_t> const & osmIds)
{
  vector<uint32_t> featureIds(osmIds.size());
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

  m_restrictions.emplace_back(type, featureIds);
  return true;
}

void RestrictionCollector::AddFeatureId(uint32_t featureId, vector<uint64_t> const & osmIds)
{
  // Note. One |featureId| could correspond to several osm ids.
  // but for road feature |featureId| corresponds exactly one osm id.
  for (uint64_t const & osmId : osmIds)
  {
    auto const result = m_osmIdToFeatureId.insert(make_pair(osmId, featureId));
    if (result.second == false)
    {
      LOG(LERROR, ("Osm id", osmId, "is included in two feature ids: ", featureId,
                   m_osmIdToFeatureId.find(osmId)->second));
    }
  }
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
}  // namespace routing
