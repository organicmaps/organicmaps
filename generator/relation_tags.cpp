#include "generator/relation_tags.hpp"

#include "generator/osm_element.hpp"

#include "base/stl_helpers.hpp"

namespace generator
{
RelationTagsBase::RelationTagsBase() : m_cache(14 /* logCacheSize */) {}

void RelationTagsBase::Reset(uint64_t fID, OsmElement * p)
{
  m_featureID = fID;
  m_current = p;
}

bool RelationTagsBase::IsSkipRelation(std::string const & type)
{
  /// @todo Skip special relation types.
  return type == "multipolygon" || type == "bridge";
}

bool RelationTagsBase::IsKeyTagExists(std::string const & key) const
{
  auto const & tags = m_current->m_tags;
  return base::AnyOf(tags, [&](OsmElement::Tag const & p) { return p.m_key == key; });
}

void RelationTagsBase::AddCustomTag(std::pair<std::string, std::string> const & p)
{
  m_current->AddTag(p.first, p.second);
}

void RelationTagsNode::Process(RelationElement const & e)
{
  std::string const & type = e.GetType();
  if (Base::IsSkipRelation(type))
    return;

  bool const processAssociatedStreet = type == "associatedStreet" &&
                                       Base::IsKeyTagExists("addr:housenumber") &&
                                       !Base::IsKeyTagExists("addr:street");
  for (auto const & p : e.m_tags)
  {
    // - used in railway station processing
    // - used in routing information
    // - used in building addresses matching
    if (p.first == "network" || p.first == "operator" || p.first == "route" ||
        p.first == "maxspeed" ||
        strings::StartsWith(p.first, "addr:"))
    {
      if (!Base::IsKeyTagExists(p.first))
        Base::AddCustomTag(p);
    }
    // Convert associatedStreet relation name to addr:street tag if we don't have one.
    else if (p.first == "name" && processAssociatedStreet)
      Base::AddCustomTag({"addr:street", p.second});
  }
}

bool RelationTagsWay::IsAcceptBoundary(RelationElement const & e) const
{
  std::string role;
  CHECK(e.FindWay(Base::m_featureID, role), (Base::m_featureID));

  // Do not accumulate boundary types (boundary=administrative) for inner polygons.
  // Example: Minsk city border (admin_level=8) is inner for Minsk area border (admin_level=4).
  return role != "inner";
}

void RelationTagsWay::Process(RelationElement const & e)
{
  /// @todo Review route relations in future.
  /// Actually, now they give a lot of dummy tags.
  std::string const & type = e.GetType();
  if (Base::IsSkipRelation(type))
    return;

  if (type == "route")
  {
    if (e.GetTagValue("route") == "road")
    {
      // Append "network/ref" to the feature ref tag.
      std::string ref = e.GetTagValue("ref");
      if (!ref.empty())
      {
        std::string const & network = e.GetTagValue("network");
        // Not processing networks with more than 15 chars (see road_shields_parser.cpp).
        if (!network.empty() && network.find('/') == std::string::npos && network.size() < 15)
          ref = network + '/' + ref;
        std::string const & refBase = m_current->GetTag("ref");
        if (!refBase.empty())
          ref = refBase + ';' + ref;
        Base::AddCustomTag({"ref", std::move(ref)});
      }
    }
    return;
  }

  if (type == "building")
    return;

  bool const isBoundary = (type == "boundary") && IsAcceptBoundary(e);
  bool const processAssociatedStreet = type == "associatedStreet" &&
                                       Base::IsKeyTagExists("addr:housenumber") &&
                                       !Base::IsKeyTagExists("addr:street");
  bool const isHighway = Base::IsKeyTagExists("highway");

  for (auto const & p : e.m_tags)
  {
    /// @todo Skip common key tags.
    if (p.first == "type" || p.first == "route" || p.first == "area")
      continue;

    // Convert associatedStreet relation name to addr:street tag if we don't have one.
    if (p.first == "name" && processAssociatedStreet)
      Base::AddCustomTag({"addr:street", p.second});

    // All "name" tags should be skipped.
    if (strings::StartsWith(p.first, "name") || strings::StartsWith(p.first, "int_name") ||
        strings::StartsWith(p.first, "old_name") || strings::StartsWith(p.first, "alt_name"))
    {
      continue;
    }

    if (!isBoundary && p.first == "boundary")
      continue;

    if (p.first == "place")
      continue;

    // Do not pass "ref" tags from boundaries and other, non-route relations to highways.
    if (p.first == "ref" && isHighway)
      continue;

    Base::AddCustomTag(p);
  }
}
}  // namespace generator
