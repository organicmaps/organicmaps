#pragma once

#include "base/math.hpp"
#include "base/string_utils.hpp"

#include "std/exception.hpp"
#include "std/function.hpp"
#include "std/iomanip.hpp"
#include "std/iostream.hpp"
#include "std/map.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

struct XMLElement
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
    string role;

    Member() = default;
    Member(uint64_t ref, EntityType type, string const & role)
    : ref(ref), type(type), role(role)
    {}

    bool operator == (Member const & e) const
    {
      return ref == e.ref && type == e.type && role == e.role;
    }
  };

  struct Tag
  {
    string key;
    string value;

    Tag() = default;
    Tag(string const & k, string const & v) : key(k), value(v) {}

    bool operator == (Tag const & e) const
    {
      return key == e.key && value == e.value;
    }
  };

  EntityType type = EntityType::Unknown;
  uint64_t id = 0;
  double lon = 0;
  double lat = 0;
  uint64_t ref = 0;
  string k;
  string v;
  EntityType memberType = EntityType::Unknown;
  string role;

  vector<uint64_t> m_nds;
  vector<Member> m_members;
  vector<Tag> m_tags;

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

  string ToString(string const & shift = string()) const;

  inline vector<uint64_t> const & Nodes() const { return m_nds; }
  inline vector<Member> const & Members() const { return m_members; }
  inline vector<Tag> const & Tags() const { return m_tags; }

  static EntityType StringToEntityType(string const & t)
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

  bool operator == (XMLElement const & e) const
  {
    return (
            type == e.type
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
            && m_tags == e.m_tags
    );
  }

  void AddTag(string const & k, string const & v) { m_tags.emplace_back(k, v); }
  void AddNd(uint64_t ref) { m_nds.emplace_back(ref); }
  void AddMember(uint64_t ref, EntityType type, string const & role)
  {
    m_members.emplace_back(ref, type, role);
  }
};

string DebugPrint(XMLElement const & e);

class BaseOSMParser
{
  XMLElement m_parent;
  XMLElement m_child;

  size_t m_depth = 0;
  XMLElement * m_current = nullptr;

  using TEmmiterFn = function<void(XMLElement *)>;
  TEmmiterFn m_EmmiterFn;

public:
  BaseOSMParser(TEmmiterFn fn) : m_EmmiterFn(fn) {}

  void CharData(string const &) {}

  void AddAttr(string const & key, string const & value)
  {
    if (!m_current)
      return;

    if (key == "id")
      CHECK ( strings::to_uint64(value, m_current->id), ("Unknown element with invalid id : ", value) );
    else if (key == "lon")
      CHECK ( strings::to_double(value, m_current->lon), ("Bad node lon : ", value) );
    else if (key == "lat")
      CHECK ( strings::to_double(value, m_current->lat), ("Bad node lat : ", value) );
    else if (key == "ref")
      CHECK ( strings::to_uint64(value, m_current->ref), ("Bad node ref in way : ", value) );
    else if (key == "k")
      m_current->k = value;
    else if (key == "v")
      m_current->v = value;
    else if (key == "type")
      m_current->memberType = XMLElement::StringToEntityType(value);
    else if (key == "role")
      m_current->role = value;
  }

  bool Push(string const & tagName)
  {
    ASSERT_GREATER_OR_EQUAL(tagName.size(), 2, ());
    
    // As tagKey we use first two char of tag name.
    XMLElement::EntityType tagKey = XMLElement::EntityType(*reinterpret_cast<uint16_t const *>(tagName.data()));

    switch (++m_depth)
    {
      case 1:
        m_current = nullptr;
        break;
      case 2:
        m_current = &m_parent;
        m_current->type = tagKey;
        break;
      default:
        m_current = &m_child;
        m_current->type = tagKey;
    }
    return true;
  }

  void Pop(string const & v)
  {
    switch (--m_depth)
    {
      case 0:
        break;

      case 1:
        m_EmmiterFn(m_current);
        m_parent.Clear();
        break;

      default:
        switch (m_child.type)
        {
          case XMLElement::EntityType::Member:
            m_parent.AddMember(m_child.ref, m_child.memberType, m_child.role);
            break;
          case XMLElement::EntityType::Tag:
            m_parent.AddTag(m_child.k, m_child.v);
            break;
          case XMLElement::EntityType::Nd:
            m_parent.AddNd(m_child.ref);
          default: break;
        }
        m_current = &m_parent;
        m_child.Clear();
    }
  }
};
