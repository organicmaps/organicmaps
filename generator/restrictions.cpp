#include "generator/restrictions.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/fstream.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

namespace
{
string const kNoStr = "No";
string const kOnlyStr = "Only";
}  // namespace

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

void RestrictionCollector::AddRestriction(vector<osm::Id> const & links, Type type)
{
  lock_guard<mutex> lock(m_mutex);
  m_restrictions.emplace_back(type, links.size());
  size_t const restrictionCount = m_restrictions.size() - 1;
  for (size_t i = 0; i < links.size(); ++i)
    m_restrictionIndex.emplace_back(links[i], Index({restrictionCount, i}));
}

void RestrictionCollector::AddFeatureId(vector<osm::Id> const & osmIds, FeatureId featureId)
{
  // Note. One |featureId| could correspond to several osm ids.
  // but for road feature |featureId| corresponds exactly one osm id.
  lock_guard<mutex> lock(m_mutex);
  for (osm::Id const & osmId : osmIds)
    m_osmIds2FeatureId.insert(make_pair(osmId, featureId));
}

bool RestrictionCollector::CheckCorrectness() const
{  
  return find_if(begin(m_restrictions), end(m_restrictions),
                                            [](Restriction const & r){ return !r.IsValid(); })
      == end(m_restrictions);
}

void RestrictionCollector::RemoveInvalidRestrictions()
{
  m_restrictions.erase(remove_if(m_restrictions.begin(), m_restrictions.end(),
                                 [](Restriction const & r){ return !r.IsValid(); }),
      m_restrictions.end());
}

void RestrictionCollector::ComposeRestrictions()
{
  // Going throught all osm id saved in |m_restrictionIndex| (mentioned in restrictions).
  size_t const restrictionSz = m_restrictions.size();
  for (pair<osm::Id, Index> const & osmIdAndIndex : m_restrictionIndex)
  {
    Index const & index = osmIdAndIndex.second;
    CHECK_LESS(index.m_restrictionNumber, restrictionSz, ());
    Restriction & restriction = m_restrictions[index.m_restrictionNumber];
    CHECK_LESS(index.m_linkNumber, restriction.m_links.size(), ());

    osm::Id const & osmId = osmIdAndIndex.first;
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

void RestrictionCollector::ComposeRestrictionsAndSave(string const & fullPath)
{
  lock_guard<mutex> lock(m_mutex);

  ComposeRestrictions();
  RemoveInvalidRestrictions();

  if (m_restrictions.empty())
    return;

  LOG(LINFO, ("Saving intermediate file with restrictions to", fullPath));
  ofstream ofs(fullPath, std::ofstream::out);
  if (ofs.fail())
  {
    LOG(LERROR, ("Cannot open", fullPath, "while saving road restrictions in intermediate format."));
    return;
  }

  for (Restriction const & r : m_restrictions)
  {
    ofs << ToString(r.m_type) << ", ";
    for (FeatureId fid : r.m_links)
      ofs << fid << ", ";
    ofs << endl;
  }
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

bool FromString(string const & str, RestrictionCollector::Type & type)
{
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

string DebugPrint(RestrictionCollector::Restriction const & restriction)
{
  ostringstream out;
  out << "m_links:[" << DebugPrint(restriction.m_links) << "] m_type:" << DebugPrint(restriction.m_type) << " ";
  return out.str();
}
