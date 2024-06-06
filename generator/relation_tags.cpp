#include "generator/relation_tags.hpp"

#include "generator/osm_element.hpp"

#include "indexer/classificator.hpp"

#include "base/string_utils.hpp"


namespace generator
{
RelationTagsBase::RelationTagsBase() : m_cache(14 /* logCacheSize */) {}

void RelationTagsBase::Reset(uint64_t fID, OsmElement * p)
{
  m_featureID = fID;
  m_current = p;
}

bool RelationTagsBase::IsSkipRelation(std::string_view type)
{
  /// @todo Skip special relation types.
  return type == "multipolygon" || type == "bridge";
}

bool RelationTagsBase::IsKeyTagExists(std::string_view const & key) const
{
  return m_current->HasTag(key);
}

void RelationTagsBase::AddCustomTag(std::string_view key, std::string_view value)
{
  /// @todo UpdateTag is better here, because caller doesn't always make IsKeyTagExists check ?!
  /// I suspect that it works ok now, because duplicating key tag is added to the end of tags vector
  /// and GetNameAndType function grabs it last.
  m_current->AddTag(key, value);
}

void RelationTagsBase::AddTagIfNotExist(std::string_view key, std::string_view value)
{
  if (!m_current->HasTag(key))
    m_current->AddTag(key, value);
}

void RelationTagsNode::Process(RelationElement const & e)
{
  auto const type = e.GetType();
  if (Base::IsSkipRelation(type))
    return;

  bool const isBoundary = (type == "boundary");
  bool const isPlaceDest = Base::IsKeyTagExists("place") || Base::IsKeyTagExists("de:place");
  bool const processAssociatedStreet = type == "associatedStreet" &&
                                       Base::IsKeyTagExists("addr:housenumber") &&
                                       !Base::IsKeyTagExists("addr:street");
  for (auto const & p : e.m_tags)
  {
    // - used in railway station processing
    // - used in routing information
    // - used in building addresses matching
    if (p.first == "network" || p.first == "operator" || p.first == "route" ||
        p.first == "maxspeed" || p.first.starts_with("addr:"))
    {
      if (!Base::IsKeyTagExists(p.first))
        Base::AddCustomTag(p);
    }
    else if (p.first == "name" && processAssociatedStreet)
    {
      // Convert associatedStreet relation name to addr:street tag if we don't have one.
      Base::AddCustomTag("addr:street", p.second);
    }
    else if (isBoundary && isPlaceDest && (p.first == "wikipedia" || p.first == "wikidata"))
    {
      if (!Base::IsKeyTagExists(p.first))
        Base::AddCustomTag(p);
    }
  }
}

bool RelationTagsWay::IsAcceptBoundary(RelationElement const & e) const
{
  // Do not accumulate boundary types (boundary=administrative) for inner polygons.
  // Example: Minsk city border (admin_level=8) is inner for Minsk area border (admin_level=4).
  if (e.GetWayRole(Base::m_featureID) == "inner")
    return false;

  // Skip religious_administration, political, etc ...
  // https://github.com/organicmaps/organicmaps/issues/4702
  auto const v = e.GetTagValue("boundary");
  return (!v.empty () && classif().GetTypeByPathSafe({"boundary", v}) != Classificator::INVALID_TYPE);
}

void RelationTagsWay::Process(RelationElement const & e)
{
  // https://github.com/organicmaps/organicmaps/issues/4051
  /// @todo We skip useful Linear tags. Put workaround now and review *all* this logic in future!
  /// Should parse classifier types for Relations separately and combine classifier types
  /// with Nodes and Ways, according to the drule's geometry type.
  auto const barrier = e.GetTagValue("barrier");
  if (!barrier.empty())
    Base::AddCustomTag("barrier", barrier);

  auto const type = e.GetType();
  if (Base::IsSkipRelation(type))
    return;

  bool const isHighway = Base::IsKeyTagExists("highway");

  /// @todo Review route relations in future. Actually, now they give a lot of dummy tags.
  if (type == "route")
  {
    auto const route = e.GetTagValue("route");
    bool fetchTags = (isHighway && route == "road");

    if (fetchTags)
    {
      /* Road ref format is
       *    8;e-road/E 67;ee:local/7841171
       * where road refs/shields are separated by a ";"
       * with an optionally prepended network code followed by a "/".
       * Its parsed by indexer/road_shields_parser.cpp.
       */
      std::string ref(e.GetTagValue("ref"));
      if (!ref.empty())
      {
        auto refBase = m_current->GetTag("ref");
        if (refBase == ref)
          refBase.clear();

        auto const network = e.GetTagValue("network");
        // Not processing networks with more than 15 chars (see road_shields_parser.cpp).
        if (!network.empty() && network.find('/') == std::string::npos && network.size() < 15)
          ref = std::string(network).append(1, '/').append(ref);

        if (!refBase.empty())
          ref = refBase + ';' + ref;

        Base::AddCustomTag("ref", ref);
      }
    }
    else
    {
      // Pass this route Relation forward to fetch it's tags (like foot, bicycle, ..., wikipedia).
      fetchTags = (route == "ferry" || (route == "train" && !e.GetTagValue("shuttle").empty()));
    }

    if (isHighway)
    {
      if (route == "bicycle")
        Base::AddTagIfNotExist("bicycle", "yes");
      else if (route == "foot" || route == "hiking")
        Base::AddTagIfNotExist("foot", "yes");
    }

    if (!fetchTags)
      return;
  }

  if (type == "building")
    return;

  bool isBoundary = false;
  if (type == "boundary")
  {
    if (!IsAcceptBoundary(e))
      return;
    isBoundary = true;
  }

  bool const isPlaceDest = Base::IsKeyTagExists("place") || Base::IsKeyTagExists("de:place");
  bool const isAssociatedStreet = type == "associatedStreet";
  bool const processAssociatedStreet = isAssociatedStreet &&
                                       Base::IsKeyTagExists("addr:housenumber") &&
                                       !Base::IsKeyTagExists("addr:street");

  for (auto const & p : e.m_tags)
  {
    /// @todo Skip common key tags.
    if (p.first == "type" || p.first == "area")
      continue;

    /// @todo We can't assign whole Relation's duration to the each Way. Split on Ways somehow?
    /// Make separate route = ferry/train(shuttle) Relations processing with generating one Way (routing edge).
    if (p.first == "duration")
      continue;

    // Convert associatedStreet relation name to addr:street tag if we don't have one.
    if (p.first == "name" && processAssociatedStreet)
      Base::AddCustomTag("addr:street", p.second);

    // All "name" tags should be skipped.
    if (p.first.starts_with("name") || p.first.starts_with("int_name") ||
        p.first.starts_with("old_name") || p.first.starts_with("alt_name"))
    {
      continue;
    }

    if (p.first == "wikipedia" || p.first == "wikidata")
    {
      if ((isBoundary && !isPlaceDest) || (!isHighway && isAssociatedStreet) || Base::IsKeyTagExists(p.first))
        continue;
    }

    if (p.first == "place" || p.first == "de:place" || p.first == "capital")
      continue;

    // Otherwise we have a bunch of minor islands (and other stuff) inside a country with a huge search rank.
    if (p.first == "population")
      continue;

    // Do not pass "ref" tags from boundaries and other, non-route relations to highways.
    if (p.first == "ref" && isHighway)
      continue;

    Base::AddCustomTag(p);
  }
}
}  // namespace generator
