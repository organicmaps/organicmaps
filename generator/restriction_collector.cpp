#include "generator/restriction_collector.hpp"

#include "generator/routing_helpers.hpp"

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <fstream>

namespace
{
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
RestrictionCollector::RestrictionCollector(std::string const & restrictionPath,
                                           std::string const & osmIdsToFeatureIdPath)
{
  MY_SCOPE_GUARD(clean, [this](){
    m_osmIdToFeatureId.clear();
    m_restrictions.clear();
  });

  if (!ParseOsmIdToFeatureIdMapping(osmIdsToFeatureIdPath, m_osmIdToFeatureId))
  {
    LOG(LWARNING, ("An error happened while parsing feature id to osm ids mapping from file:",
                   osmIdsToFeatureIdPath));
    return;
  }

  if (!ParseRestrictions(restrictionPath))
  {
    LOG(LWARNING, ("An error happened while parsing restrictions from file:",  restrictionPath));
    return;
  }
  clean.release();

  my::SortUnique(m_restrictions);

  if (!IsValid())
    LOG(LERROR, ("Some restrictions are not valid."));
  LOG(LDEBUG, ("Number of loaded restrictions:", m_restrictions.size()));
}

bool RestrictionCollector::IsValid() const
{
  return std::find_if(begin(m_restrictions), end(m_restrictions),
                 [](Restriction const & r) { return !r.IsValid(); }) == end(m_restrictions);
}

bool RestrictionCollector::ParseRestrictions(std::string const & path)
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

    Restriction::Type type;
    if (!FromString(*iter, type))
    {
      LOG(LWARNING, ("Cannot parse a restriction type. Line:", line));
      return false;
    }

    ++iter;
    std::vector<base::GeoObjectId> osmIds;
    if (!ParseLineOfWayIds(iter, osmIds))
    {
      LOG(LWARNING, ("Cannot parse osm ids from", path));
      return false;
    }

    AddRestriction(type, osmIds);
  }
  return true;
}

bool RestrictionCollector::AddRestriction(Restriction::Type type,
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

  m_restrictions.emplace_back(type, featureIds);
  return true;
}

void RestrictionCollector::AddFeatureId(uint32_t featureId, base::GeoObjectId osmId)
{
  ::routing::AddFeatureId(osmId, featureId, m_osmIdToFeatureId);
}

bool FromString(std::string str, Restriction::Type & type)
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
