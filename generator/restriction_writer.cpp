#include "generator/restriction_writer.hpp"

#include "generator/intermediate_elements.hpp"
#include "generator/osm_id.hpp"
#include "generator/restriction_collector.hpp"

#include "routing/routing_serialization.hpp"

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
/// \returns true if conversion was successful and false otherwise.
bool TagToType(string const & tag, Restriction::Type & type)
{
  if (find(kRestrictionTypesNo.cbegin(), kRestrictionTypesNo.cend(), tag) !=
      kRestrictionTypesNo.cend())
  {
    type = Restriction::Type::No;
    return true;
  }

  if (find(kRestrictionTypesOnly.cbegin(), kRestrictionTypesOnly.cend(), tag) !=
      kRestrictionTypesOnly.cend())
  {
    type = Restriction::Type::Only;
    return true;
  }

  // Unsupported restriction type.
  return false;
}
}  // namespace

namespace routing
{
void RestrictionWriter::Open(string const & fullPath)
{
  LOG(LINFO, ("Saving road restrictions in osm id terms to", fullPath));
  m_stream.open(fullPath, std::ofstream::out);

  if (!IsOpened())
    LOG(LINFO, ("Cannot open file", fullPath));
}

bool RestrictionWriter::IsOpened() { return m_stream.is_open() && !m_stream.fail(); }

void RestrictionWriter::Write(RelationElement const & relationElement)
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
    return;

  auto const toIt = findTag(relationElement.ways, "to");
  if (toIt == relationElement.ways.cend())
    return;

  if (findTag(relationElement.nodes, "via") == relationElement.nodes.cend())
    return;

  // Extracting type of restriction.
  auto const tagIt = relationElement.tags.find("restriction");
  if (tagIt == relationElement.tags.end())
    return;

  Restriction::Type type = Restriction::Type::No;
  if (!TagToType(tagIt->second, type))
    return;

  // Adding restriction.
  m_stream << ToString(type) << "," << fromIt->first << ", " << toIt->first << '\n';
}
}  // namespace routing
