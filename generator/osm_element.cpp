#include "generator/osm_element.hpp"

#include "coding/parse_xml.hpp"

#include "base/string_utils.hpp"
#include "base/stl_helpers.hpp"

#include <cstdio>
#include <cstring>
#include <sstream>

std::string DebugPrint(OsmElement::EntityType type)
{
  switch (type)
  {
  case OsmElement::EntityType::Unknown:
    return "unknown";
  case OsmElement::EntityType::Way:
    return "way";
  case OsmElement::EntityType::Tag:
    return "tag";
  case OsmElement::EntityType::Relation:
    return "relation";
  case OsmElement::EntityType::Osm:
    return "osm";
  case OsmElement::EntityType::Node:
    return "node";
  case OsmElement::EntityType::Nd:
    return "nd";
  case OsmElement::EntityType::Member:
    return "member";
  }
  UNREACHABLE();
}

void OsmElement::AddTag(Tag const & tag) { AddTag(tag.m_key, tag.m_value); }

void OsmElement::AddTag(char const * key, char const * value)
{
  ASSERT(key, ());
  ASSERT(value, ());

  // Seems like source osm data has empty values. They are useless for us.
  if (key[0] == '\0' || value[0] == '\0')
    return;

#define SKIP_KEY_BY_PREFIX(skippedKey) if (std::strncmp(key, skippedKey, sizeof(skippedKey)-1) == 0) return;
  // OSM technical info tags
  SKIP_KEY_BY_PREFIX("created_by");
  SKIP_KEY_BY_PREFIX("source");
  SKIP_KEY_BY_PREFIX("odbl");
  SKIP_KEY_BY_PREFIX("note");
  SKIP_KEY_BY_PREFIX("fixme");
  SKIP_KEY_BY_PREFIX("iemv");

  // Skip tags for speedup, now we don't use it
  SKIP_KEY_BY_PREFIX("not:");
  SKIP_KEY_BY_PREFIX("artist_name");
  SKIP_KEY_BY_PREFIX("whitewater"); // https://wiki.openstreetmap.org/wiki/Whitewater_sports

  // In future we can use this tags for improve our search
  SKIP_KEY_BY_PREFIX("nat_name");
  SKIP_KEY_BY_PREFIX("reg_name");
  SKIP_KEY_BY_PREFIX("loc_name");
  SKIP_KEY_BY_PREFIX("lock_name");
  SKIP_KEY_BY_PREFIX("local_name");
  SKIP_KEY_BY_PREFIX("short_name");
  SKIP_KEY_BY_PREFIX("official_name");
#undef SKIP_KEY_BY_PREFIX

  std::string_view val(value);
  strings::Trim(val);
  m_tags.emplace_back(key, val);
}

void OsmElement::AddTag(std::string const & key, std::string const & value)
{
  AddTag(key.data(), value.data());
}

bool OsmElement::HasTag(std::string const & key) const
{
  return base::AnyOf(m_tags, [&](auto const & t) { return t.m_key == key; });
}

bool OsmElement::HasTag(std::string const & key, std::string const & value) const
{
  return base::AnyOf(m_tags, [&](auto const & t) { return t.m_key == key && t.m_value == value; });
}

bool OsmElement::HasAnyTag(std::unordered_multimap<std::string, std::string> const & tags) const
{
  return base::AnyOf(m_tags, [&](auto const & t) {
    auto beginEnd = tags.equal_range(t.m_key);
    for (auto it = beginEnd.first; it != beginEnd.second; ++it)
    {
      if (it->second == t.m_value)
        return true;
    }

    return false;
  });
}

std::string OsmElement::ToString(std::string const & shift) const
{
  std::stringstream ss;
  ss << (shift.empty() ? "\n" : shift);
  switch (m_type)
  {
  case EntityType::Node:
    ss << "Node: " << m_id << " (" << std::fixed << std::setw(7) << m_lat << ", " << m_lon << ")"
       << " tags: " << m_tags.size();
    break;
  case EntityType::Nd:
    ss << "Nd ref: " << m_ref;
    break;
  case EntityType::Way:
    ss << "Way: " << m_id << " nds: " << m_nodes.size() << " tags: " << m_tags.size();
    if (!m_nodes.empty())
    {
      std::string shift2 = shift;
      shift2 += shift2.empty() ? "\n  " : "  ";
      for (auto const & e : m_nodes)
        ss << shift2 << e;
    }
    break;
  case EntityType::Relation:
    ss << "Relation: " << m_id << " members: " << m_members.size() << " tags: " << m_tags.size();
    if (!m_members.empty())
    {
      std::string shift2 = shift;
      shift2 += shift2.empty() ? "\n  " : "  ";
      for (auto const & e : m_members)
        ss << shift2 << e.m_ref << " " << DebugPrint(e.m_type) << " " << e.m_role;
    }
    break;
  case EntityType::Tag:
    ss << "Tag: " << m_k << " = " << m_v;
    break;
  case EntityType::Member:
    ss << "Member: " << m_ref << " type: " << DebugPrint(m_memberType) << " role: " << m_role;
    break;
  case EntityType::Unknown:
  case EntityType::Osm:
    UNREACHABLE();
    break;
  }

  if (!m_tags.empty())
  {
    std::string shift2 = shift;
    shift2 += shift2.empty() ? "\n  " : "  ";
    for (auto const & e : m_tags)
      ss << shift2 << e.m_key << " = " << e.m_value;
  }
  return ss.str();
}

std::string OsmElement::GetTag(std::string const & key) const
{
  auto const it = base::FindIf(m_tags, [&key](Tag const & tag) { return tag.m_key == key; });
  return it == m_tags.cend() ? std::string() : it->m_value;
}

std::string OsmElement::GetTagValue(std::string const & key,
                                    std::string const & defaultValue) const
{
  auto const it = base::FindIf(m_tags, [&key](Tag const & tag) { return tag.m_key == key; });
  return it != m_tags.cend() ? it->m_value : defaultValue;
}

std::string DebugPrint(OsmElement const & element)
{
  return element.ToString();
}

std::string DebugPrint(OsmElement::Tag const & tag)
{
  std::stringstream ss;
  ss << tag.m_key << '=' << tag.m_value;
  return ss.str();
}

base::GeoObjectId GetGeoObjectId(OsmElement const & element)
{
  switch (element.m_type)
  {
  case OsmElement::EntityType::Node:
    return base::MakeOsmNode(element.m_id);
  case OsmElement::EntityType::Way:
    return base::MakeOsmWay(element.m_id);
  case OsmElement::EntityType::Relation:
    return base::MakeOsmRelation(element.m_id);
  case OsmElement::EntityType::Member:
  case OsmElement::EntityType::Nd:
  case OsmElement::EntityType::Osm:
  case OsmElement::EntityType::Tag:
  case OsmElement::EntityType::Unknown:
    UNREACHABLE();
    return base::GeoObjectId();
  }
  UNREACHABLE();
}
