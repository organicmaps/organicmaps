#include "generator/intermediate_elements.hpp"
#include "generator/restrictions.hpp"

#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include "std/algorithm.hpp"
#include "std/fstream.hpp"

namespace
{
/// \brief Converts restriction type form string to RestrictionCollector::Type.
/// \returns Fisrt item is a result of conversion. Second item is true
/// if convertion was successful and false otherwise.
pair<RestrictionCollector::Type, bool> TagToType(string const & type)
{
  vector<string> const restrictionTypesNo = {"no_right_turn", "no_left_turn", "no_u_turn",
                                             "no_straight_on", "no_entry", "no_exit"};
  vector<string> const restrictionTypesOnly = {"only_right_turn", "only_left_turn", "only_straight_on"};

  if (find(restrictionTypesNo.cbegin(), restrictionTypesNo.cend(), type) != restrictionTypesNo.cend())
    return make_pair(RestrictionCollector::Type::No, true);

  if (find(restrictionTypesOnly.cbegin(), restrictionTypesOnly.cend(), type) != restrictionTypesOnly.cend())
    return make_pair(RestrictionCollector::Type::Only, true);

  // Unsupported restriction type.
  return make_pair(RestrictionCollector::Type::No, false);
}
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
  for (auto fid : m_links)
  {
    if (fid == kInvalidFeatureId)
      return false;
  }
  return true;
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

RestrictionCollector::~RestrictionCollector() {}

void RestrictionCollector::AddRestriction(vector<osm::Id> const & links, Type type)
{
  lock_guard<mutex> lock(m_mutex);
  m_restrictions.emplace_back(type, links.size());
  size_t const restrictionCount = m_restrictions.size() - 1;
  for (size_t i = 0; i < links.size(); ++i)
    m_restrictionIndex.push_back(make_pair(links[i], Index({restrictionCount, i})));
}

void RestrictionCollector::AddRestriction(RelationElement const & relationElement)
{
  CHECK_EQUAL(relationElement.GetType(), "restriction", ());

  // Note. For the time being only line-point-line road restriction is supported.
  if (relationElement.nodes.size() != 1 || relationElement.ways.size() != 2)
    return; // Unsupported restriction. For example line-line-line.

  // Extracting osm ids of lines and points of the restriction.
  auto const findTag = [&relationElement](vector<pair<uint64_t, string>> const & members, string const & tag)
  {
    auto const it = find_if(members.cbegin(), members.cend(),
                            [&tag](pair<uint64_t, string> const & p) { return p.second == tag; });
    return it;
  };

  auto const fromIt = findTag(relationElement.ways, "from");
  if (fromIt == relationElement.ways.cend())
    return; // No tag |from| in |relationElement.ways|.

  auto const toIt = findTag(relationElement.ways, "to");
  if (toIt == relationElement.ways.cend())
    return; // No tag |to| in |relationElement.ways|.

  if (findTag(relationElement.nodes,"via") == relationElement.nodes.cend())
    return; // No tag |via| in |relationElement.nodes|.

  // Extracting type of restriction.
  auto const tagIt = relationElement.tags.find("restriction");
  if (tagIt == relationElement.tags.end())
    return; // Type of the element is different from "restriction".

  auto const typeResult = TagToType(tagIt->second);
  if (typeResult.second == false)
    return; // Unsupported restriction type.

  // Adding restriction.
  Type const type = typeResult.first;
  osm::Id const fromOsmId = osm::Id::Way(fromIt->first);
  osm::Id const toOsmId = osm::Id::Way(toIt->first);
  AddRestriction({fromOsmId, toOsmId}, type);
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
  for (Restriction const & restriction : m_restrictions)
  {
    if (!restriction.IsValid())
      return false;
  }
  return true;
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
    Index const index = osmIdAndIndex.second;
    CHECK_LESS(index.m_restrictionNumber, restrictionSz, ());
    Restriction & restriction = m_restrictions[index.m_restrictionNumber];
    CHECK_LESS(index.m_linkNumber, restriction.m_links.size(), ());

    osm::Id const osmId = osmIdAndIndex.first;
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

    FeatureId const featureId = rangeId.first->second;
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
  for (Restriction const & r : m_restrictions)
  {
    ofs << DebugPrint(r.m_type) << ", ";
    for (FeatureId fid : r.m_links)
      ofs << fid << ", ";
    ofs << endl;
  }
}

string DebugPrint(RestrictionCollector::Type const & type)
{
  switch (type)
  {
  case RestrictionCollector::Type::No:
    return "No";
  case RestrictionCollector::Type::Only:
    return "Only";
  }
  return "Unknown";
}

string DebugPrint(RestrictionCollector::Restriction const & restriction)
{
  ostringstream out;
  out << "m_links:[" << restriction.m_links << "] m_type:" << DebugPrint(restriction.m_type) << " ";
  return out.str();
}
