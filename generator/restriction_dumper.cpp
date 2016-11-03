#include "generator/restriction_dumper.hpp"
#include "generator/intermediate_elements.hpp"
#include "generator/osm_id.hpp"
#include "generator/restriction_collector.hpp"

#include "indexer/routing.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/fstream.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

namespace
{
using namespace routing;

vector<string> const kRestrictionTypesNo = {"no_right_turn",  "no_left_turn", "no_u_turn",
                                            "no_straight_on", "no_entry",     "no_exit"};
vector<string> const kRestrictionTypesOnly = {"only_right_turn", "only_left_turn",
                                              "only_straight_on"};

/// \brief Converts restriction type form string to RestrictionCollector::Type.
/// \returns Fisrt item is a result of conversion. Second item is true
/// if convertion was successful and false otherwise.
pair<Restriction::Type, bool> TagToType(string const & type)
{
  if (find(kRestrictionTypesNo.cbegin(), kRestrictionTypesNo.cend(), type) !=
      kRestrictionTypesNo.cend())
  {
    return make_pair(Restriction::Type::No, true);
  }

  if (find(kRestrictionTypesOnly.cbegin(), kRestrictionTypesOnly.cend(), type) !=
      kRestrictionTypesOnly.cend())
  {
    return make_pair(Restriction::Type::Only, true);
  }

  // Unsupported restriction type.
  return make_pair(Restriction::Type::No, false);
}
}  // namespace

namespace routing
{
void RestrictionDumper::Open(string const & fullPath)
{
  LOG(LINFO, ("Saving road restrictions in osm id terms to", fullPath));
  m_stream.open(fullPath, std::ofstream::out);

  if (!IsOpened())
    LOG(LINFO, ("Cannot open file", fullPath));
}

bool RestrictionDumper::IsOpened() { return m_stream.is_open() && !m_stream.fail(); }

void RestrictionDumper::Write(RelationElement const & relationElement)
{
  if (!IsOpened())
    return;

  CHECK_EQUAL(relationElement.GetType(), "restriction", ());

  // Note. For the time being only line-point-line road restriction is supported.
  if (relationElement.nodes.size() != 1 || relationElement.ways.size() != 2)
    return;  // Unsupported restriction. For example line-line-line.

  // Extracting osm ids of lines and points of the restriction.
  auto const findTag = [&relationElement](vector<pair<uint64_t, string>> const & members,
                                          string const & tag) {
    auto const it = find_if(members.cbegin(), members.cend(),
                            [&tag](pair<uint64_t, string> const & p) { return p.second == tag; });
    return it;
  };

  auto const fromIt = findTag(relationElement.ways, "from");
  if (fromIt == relationElement.ways.cend())
    return;  // No tag |from| in |relationElement.ways|.

  auto const toIt = findTag(relationElement.ways, "to");
  if (toIt == relationElement.ways.cend())
    return;  // No tag |to| in |relationElement.ways|.

  if (findTag(relationElement.nodes, "via") == relationElement.nodes.cend())
    return;  // No tag |via| in |relationElement.nodes|.

  // Extracting type of restriction.
  auto const tagIt = relationElement.tags.find("restriction");
  if (tagIt == relationElement.tags.end())
    return;  // Type of the element is different from "restriction".

  auto const typeResult = TagToType(tagIt->second);
  if (typeResult.second == false)
    return;  // Unsupported restriction type.

  // Adding restriction.
  m_stream << ToString(typeResult.first) << ","  // Restriction type
           << fromIt->first << ", " << toIt->first << "," << endl;
}
}  // namespace routing
