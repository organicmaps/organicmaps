#pragma once

#include "base/assert.hpp"
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
    uint64_t ref = 0;
    EntityType type = EntityType::Unknown;
    std::string role;

    Member() = default;
    Member(uint64_t ref, EntityType type, std::string const & role)
    : ref(ref), type(type), role(role)
    {}

    bool operator == (Member const & e) const
    {
      return ref == e.ref && type == e.type && role == e.role;
    }
  };

  struct Tag
  {
    std::string key;
    std::string value;

    Tag() = default;
    Tag(std::string const & k, std::string const & v) : key(k), value(v) {}

    bool operator == (Tag const & e) const
    {
      return key == e.key && value == e.value;
    }
    bool operator < (Tag const & e) const
    {
      if (key == e.key)
        return value < e.value;
      return key < e.key;
    }
  };

  EntityType type = EntityType::Unknown;
  uint64_t id = 0;
  double lon = 0;
  double lat = 0;
  uint64_t ref = 0;
  std::string k;
  std::string v;
  EntityType memberType = EntityType::Unknown;
  std::string role;

  std::vector<uint64_t> m_nds;
  std::vector<Member> m_members;
  std::vector<Tag> m_tags;

  void Clear()
  {
    type = EntityType::Unknown;
    id = 0;
    lon = 0;
    lat = 0;
    ref = 0;
    k.clear();
    v.clear();
    memberType = EntityType::Unknown;
    role.clear();

    m_nds.clear();
    m_members.clear();
    m_tags.clear();
  }

  std::string ToString(std::string const & shift = std::string()) const;

  std::vector<uint64_t> const & Nodes() const { return m_nds; }
  std::vector<Member> const & Members() const { return m_members; }
  std::vector<Tag> const & Tags() const { return m_tags; }

  bool IsNode() const { return type == EntityType::Node; }
  bool IsWay() const { return type == EntityType::Way; }
  bool IsRelation() const { return type == EntityType::Relation; }

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

  bool operator==(OsmElement const & e) const
  {
    return type == e.type
            && id == e.id
            && my::AlmostEqualAbs(lon, e.lon, 1e-7)
            && my::AlmostEqualAbs(lat, e.lat, 1e-7)
            && ref == e.ref
            && k == e.k
            && v == e.v
            && memberType == e.memberType
            && role == e.role
            && m_nds == e.m_nds
            && m_members == e.m_members
            && m_tags == e.m_tags;
  }

  void AddNd(uint64_t ref) { m_nds.emplace_back(ref); }
  void AddMember(uint64_t ref, EntityType type, std::string const & role)
  {
    m_members.emplace_back(ref, type, role);
  }

  void AddTag(std::string const & k, std::string const & v);
  bool HasTagValue(std::string const & k, std::string const & v) const;

  template <class Fn>
  void UpdateTag(std::string const & k, Fn && fn)
  {
    for (auto & tag : m_tags)
    {
      if (tag.key == k)
      {
        fn(tag.value);
        return;
      }
    }

    std::string v;
    fn(v);
    if (!v.empty())
      AddTag(k, v);
  }

  std::string GetTag(std::string const & key) const;
};

std::string DebugPrint(OsmElement const & e);
std::string DebugPrint(OsmElement::EntityType e);
std::string DebugPrint(OsmElement::Tag const & tag);
