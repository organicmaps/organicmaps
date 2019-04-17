#pragma once

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"
#include "base/math.hpp"
#include "base/string_utils.hpp"

#include "std/string_view.hpp"

#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct OsmElement
{
  enum class EntityType
  {
    Unknown = 0x0,
    Node = 0x6F6E, // "no"
    Way = 0x6177, // "wa"
    Relation = 0x6572, // "re"
    Tag = 0x6174, // "ta"
    Nd = 0x646E, // "nd"
    Member = 0x656D, // "me"
    Osm = 0x736F, // "os"
  };

  struct Member
  {
    Member() = default;
    Member(uint64_t ref, EntityType type, std::string const & role)
      : m_ref(ref), m_type(type), m_role(role) {}

    bool operator==(Member const & e) const
    {
      return m_ref == e.m_ref && m_type == e.m_type && m_role == e.m_role;
    }

    uint64_t m_ref = 0;
    EntityType m_type = EntityType::Unknown;
    std::string m_role;
  };

  struct Tag
  {
    Tag() = default;
    Tag(std::string const & k, std::string const & v) : m_key(k), m_value(v) {}

    bool operator==(Tag const & e) const
    {
      return m_key == e.m_key && m_value == e.m_value;
    }

    bool operator<(Tag const & e) const
    {
      return m_key == e.m_key ?  m_value < e.m_value : m_key < e.m_key;
    }

    std::string m_key;
    std::string m_value;
  };

  static EntityType StringToEntityType(std::string const & t)
  {
    if (t == "way")
      return EntityType::Way;
    if (t == "node")
      return EntityType::Node;
    if (t == "relation")
      return EntityType::Relation;
    ASSERT(false, ("Unknown type", t));
    return EntityType::Unknown;
  }

  void Clear()
  {
    m_type = EntityType::Unknown;
    m_id = 0;
    m_lon = 0;
    m_lat = 0;
    m_ref = 0;
    m_k.clear();
    m_v.clear();
    m_memberType = EntityType::Unknown;
    m_role.clear();

    m_nds.clear();
    m_members.clear();
    m_tags.clear();
  }

  std::string ToString(std::string const & shift = std::string()) const;

  std::vector<uint64_t> const & Nodes() const { return m_nds; }
  std::vector<Member> const & Members() const { return m_members; }
  std::vector<Tag> const & Tags() const { return m_tags; }

  bool IsNode() const { return m_type == EntityType::Node; }
  bool IsWay() const { return m_type == EntityType::Way; }
  bool IsRelation() const { return m_type == EntityType::Relation; }

  bool operator==(OsmElement const & e) const
  {
    return m_type == e.m_type
        && m_id == e.m_id
        && base::AlmostEqualAbs(m_lon, e.m_lon, 1e-7)
        && base::AlmostEqualAbs(m_lat, e.m_lat, 1e-7)
        && m_ref == e.m_ref
        && m_k == e.m_k
        && m_v == e.m_v
        && m_memberType == e.m_memberType
        && m_role == e.m_role
        && m_nds == e.m_nds
        && m_members == e.m_members
        && m_tags == e.m_tags;
  }

  void AddNd(uint64_t ref) { m_nds.emplace_back(ref); }
  void AddMember(uint64_t ref, EntityType type, std::string const & role)
  {
    m_members.emplace_back(ref, type, role);
  }

  void AddTag(std::string_view const & k, std::string_view const & v);
  bool HasTag(std::string_view const & key) const;
  bool HasTag(std::string_view const & k, std::string_view const & v) const;
  bool HasAnyTag(std::unordered_multimap<std::string, std::string> const & tags) const;

  template <class Fn>
  void UpdateTag(std::string const & k, Fn && fn)
  {
    for (auto & tag : m_tags)
    {
      if (tag.m_key == k)
      {
        fn(tag.m_value);
        return;
      }
    }

    std::string v;
    fn(v);
    if (!v.empty())
      AddTag(k, v);
  }

  std::string GetTag(std::string const & key) const;
  std::string_view GetTagValue(std::string_view const & key, std::string_view const & defaultValue) const;
  EntityType m_type = EntityType::Unknown;
  uint64_t m_id = 0;
  double m_lon = 0;
  double m_lat = 0;
  uint64_t m_ref = 0;
  std::string m_k;
  std::string m_v;
  EntityType m_memberType = EntityType::Unknown;
  std::string m_role;

  std::vector<uint64_t> m_nds;
  std::vector<Member> m_members;
  std::vector<Tag> m_tags;
};

base::GeoObjectId GetGeoObjectId(OsmElement const & element);

std::string DebugPrint(OsmElement const & e);
std::string DebugPrint(OsmElement::EntityType e);
std::string DebugPrint(OsmElement::Tag const & tag);
