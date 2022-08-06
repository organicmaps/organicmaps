#pragma once

#include "geometry/mercator.hpp"  // kPointEqualityEps

#include "base/assert.hpp"
#include "base/geo_object_id.hpp"
#include "base/math.hpp"
#include "base/string_utils.hpp"

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

    bool operator==(Member const & other) const
    {
      return m_ref == other.m_ref && m_type == other.m_type && m_role == other.m_role;
    }

    uint64_t m_ref = 0;
    EntityType m_type = EntityType::Unknown;
    std::string m_role;
  };

  struct Tag
  {
    Tag() = default;
    Tag(std::string_view key, std::string_view value) : m_key(key), m_value(value) {}

    bool operator==(Tag const & other) const
    {
      return m_key == other.m_key && m_value == other.m_value;
    }

    bool operator<(Tag const & other) const
    {
      return m_key == other.m_key ? m_value < other.m_value : m_key < other.m_key;
    }

    std::string m_key;
    std::string m_value;
  };

  static EntityType StringToEntityType(std::string const & type)
  {
    if (type == "way")
      return EntityType::Way;
    if (type == "node")
      return EntityType::Node;
    if (type == "relation")
      return EntityType::Relation;
    ASSERT(false, ("Unknown type", type));
    return EntityType::Unknown;
  }

  void Clear()
  {
    m_type = EntityType::Unknown;
    m_id = 0;
    m_lon = 0.0;
    m_lat = 0.0;
    m_ref = 0;
    m_k.clear();
    m_v.clear();
    m_memberType = EntityType::Unknown;
    m_role.clear();

    m_nodes.clear();
    m_members.clear();
    m_tags.clear();
  }

  std::string ToString(std::string const & shift = std::string()) const;

  std::vector<uint64_t> const & Nodes() const { return m_nodes; }
  std::vector<Member> const & Members() const { return m_members; }
  std::vector<Member> & Members() { return m_members; }
  std::vector<Tag> const & Tags() const { return m_tags; }
  std::vector<Tag> & Tags() { return m_tags; }

  bool IsNode() const { return m_type == EntityType::Node; }
  bool IsWay() const { return m_type == EntityType::Way; }
  bool IsRelation() const { return m_type == EntityType::Relation; }

  bool operator==(OsmElement const & other) const
  {
    return m_type == other.m_type
        && m_id == other.m_id
        && base::AlmostEqualAbs(m_lon, other.m_lon, mercator::kPointEqualityEps)
        && base::AlmostEqualAbs(m_lat, other.m_lat, mercator::kPointEqualityEps)
        && m_ref == other.m_ref
        && m_k == other.m_k
        && m_v == other.m_v
        && m_memberType == other.m_memberType
        && m_role == other.m_role
        && m_nodes == other.m_nodes
        && m_members == other.m_members
        && m_tags == other.m_tags;
  }

  void AddNd(uint64_t ref) { m_nodes.emplace_back(ref); }
  void AddMember(uint64_t ref, EntityType type, std::string const & role)
  {
    m_members.emplace_back(ref, type, role);
  }

  void AddTag(Tag const & tag);
  void AddTag(char const * key, char const * value);
  void AddTag(std::string const & key, std::string const & value);
  bool HasTag(std::string const & key) const;
  bool HasTag(std::string const & key, std::string const & value) const;
  bool HasAnyTag(std::unordered_multimap<std::string, std::string> const & tags) const;

  template <class Fn>
  void UpdateTag(std::string const & key, Fn && fn)
  {
    for (auto & tag : m_tags)
    {
      if (tag.m_key == key)
      {
        fn(tag.m_value);
        return;
      }
    }

    std::string value;
    fn(value);
    if (!value.empty())
      AddTag(key, value);
  }

  std::string GetTag(std::string const & key) const;
  std::string GetTagValue(std::string const & key, std::string const & defaultValue) const;
  EntityType m_type = EntityType::Unknown;
  uint64_t m_id = 0;
  double m_lon = 0;
  double m_lat = 0;
  uint64_t m_ref = 0;
  std::string m_k;
  std::string m_v;
  EntityType m_memberType = EntityType::Unknown;
  std::string m_role;

  std::vector<uint64_t> m_nodes;
  std::vector<Member> m_members;
  std::vector<Tag> m_tags;
};

base::GeoObjectId GetGeoObjectId(OsmElement const & element);

std::string DebugPrint(OsmElement const & element);
std::string DebugPrint(OsmElement::EntityType type);
std::string DebugPrint(OsmElement::Tag const & tag);
