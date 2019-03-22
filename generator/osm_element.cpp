#include "generator/osm_element.hpp"

#include "base/string_utils.hpp"
#include "coding/parse_xml.hpp"

#include <algorithm>
#include <cstdio>
#include <sstream>

std::string DebugPrint(OsmElement::EntityType e)
{
  switch (e)
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


void OsmElement::AddTag(std::string const & k, std::string const & v)
{
  // Seems like source osm data has empty values. They are useless for us.
  if (k.empty() || v.empty())
    return;

#define SKIP_KEY(key) if (strncmp(k.data(), key, sizeof(key)-1) == 0) return;
  // OSM technical info tags
  SKIP_KEY("created_by");
  SKIP_KEY("source");
  SKIP_KEY("odbl");
  SKIP_KEY("note");
  SKIP_KEY("fixme");
  SKIP_KEY("iemv");

  // Skip tags for speedup, now we don't use it
  SKIP_KEY("not:");
  SKIP_KEY("artist_name");
  SKIP_KEY("whitewater"); // https://wiki.openstreetmap.org/wiki/Whitewater_sports


  // In future we can use this tags for improve our search
  SKIP_KEY("old_name");
  SKIP_KEY("alt_name");
  SKIP_KEY("nat_name");
  SKIP_KEY("reg_name");
  SKIP_KEY("loc_name");
  SKIP_KEY("lock_name");
  SKIP_KEY("local_name");
  SKIP_KEY("short_name");
  SKIP_KEY("official_name");
#undef SKIP_KEY

  std::string value = v;
  strings::Trim(value);
  m_tags.emplace_back(k, value);
}

bool OsmElement::HasTag(std::string const & k, std::string const & v) const
{
  return std::any_of(m_tags.begin(), m_tags.end(), [&](auto const & t) {
    return t.key == k && t.value == v;
  });
}

bool OsmElement::HasAnyTag(std::unordered_multimap<std::string, std::string> const & tags) const
{
  return std::any_of(std::begin(m_tags), std::end(m_tags), [&](auto const & t) {
    auto beginEnd = tags.equal_range(t.key);
    for (auto it = beginEnd.first; it != beginEnd.second; ++it)
    {
      if (it->second == t.value)
        return true;
    }

    return false;
  });
}

std::string OsmElement::ToString(std::string const & shift) const
{
  std::stringstream ss;
  ss << (shift.empty() ? "\n" : shift);
  switch (type)
  {
  case EntityType::Node:
    ss << "Node: " << id << " (" << std::fixed << std::setw(7) << lat << ", " << lon << ")"
       << " tags: " << m_tags.size();
    break;
  case EntityType::Nd:
    ss << "Nd ref: " << ref;
    break;
  case EntityType::Way:
    ss << "Way: " << id << " nds: " << m_nds.size() << " tags: " << m_tags.size();
    if (!m_nds.empty())
    {
      std::string shift2 = shift;
      shift2 += shift2.empty() ? "\n  " : "  ";
      for (auto const & e : m_nds)
        ss << shift2 << e;
    }
    break;
  case EntityType::Relation:
    ss << "Relation: " << id << " members: " << m_members.size() << " tags: " << m_tags.size();
    if (!m_members.empty())
    {
      std::string shift2 = shift;
      shift2 += shift2.empty() ? "\n  " : "  ";
      for (auto const & e : m_members)
        ss << shift2 << e.ref << " " << DebugPrint(e.type) << " " << e.role;
    }
    break;
  case EntityType::Tag:
    ss << "Tag: " << k << " = " << v;
    break;
  case EntityType::Member:
    ss << "Member: " << ref << " type: " << DebugPrint(memberType) << " role: " << role;
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
      ss << shift2 << e.key << " = " << e.value;
  }
  return ss.str();
}

std::string OsmElement::GetTag(std::string const & key) const
{
  auto const it = std::find_if(m_tags.cbegin(), m_tags.cend(),
                               [&key](Tag const & tag) { return tag.key == key; });

  return it == m_tags.cend() ? std::string() : it->value;
}

std::string DebugPrint(OsmElement const & e)
{
  return e.ToString();
}

std::string DebugPrint(OsmElement::Tag const & tag)
{
  std::stringstream ss;
  ss << tag.key << '=' << tag.value;
  return ss.str();
}

base::GeoObjectId GetGeoObjectId(OsmElement const & element)
{
  switch (element.type)
  {
  case OsmElement::EntityType::Node:
    return base::MakeOsmNode(element.id);
  case OsmElement::EntityType::Way:
    return base::MakeOsmWay(element.id);
  case OsmElement::EntityType::Relation:
    return base::MakeOsmRelation(element.id);
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
