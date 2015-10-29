#include "generator/osm_element.hpp"

#include "coding/parse_xml.hpp"

#include "std/cstdio.hpp"
#include "std/algorithm.hpp"


string DebugPrint(OsmElement::EntityType e)
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
}


void OsmElement::AddTag(string const & k, string const & v)
{
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
  SKIP_KEY("whitewater"); // http://wiki.openstreetmap.org/wiki/Whitewater_sports


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

  m_tags.emplace_back(k, v);
}


string OsmElement::ToString(string const & shift) const
{
  stringstream ss;
  ss << (shift.empty() ? "\n" : shift);
  switch (type)
  {
    case EntityType::Node:
      ss << "Node: " << id << " (" << fixed << setw(7) << lat << ", " << lon << ")"
         << " tags: " << m_tags.size();
      break;
    case EntityType::Nd:
      ss << "Nd ref: " << ref;
      break;
    case EntityType::Way:
      ss << "Way: " << id << " nds: " << m_nds.size() << " tags: " << m_tags.size();
      if (!m_nds.empty())
      {
        string shift2 = shift;
        shift2 += shift2.empty() ? "\n  " : "  ";
        for (auto const & e : m_nds)
          ss << shift2 << e;
      }
      break;
    case EntityType::Relation:
      ss << "Relation: " << id << " members: " << m_members.size() << " tags: " << m_tags.size();
      if (!m_members.empty())
      {
        string shift2 = shift;
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
    default:
      ss << "Unknown element";
  }
  if (!m_tags.empty())
  {
    string shift2 = shift;
    shift2 += shift2.empty() ? "\n  " : "  ";
    for (auto const & e : m_tags)
      ss << shift2 << e.key << " = " << e.value;
  }
  return ss.str();
}

string DebugPrint(OsmElement const & e)
{
  return e.ToString();
}
