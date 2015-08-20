#pragma once

#include "base/assert.hpp"
#include "base/std_serialization.hpp"

#include "std/algorithm.hpp"
#include "std/bind.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

#define NODES_FILE "nodes.dat"
#define WAYS_FILE "ways.dat"
#define RELATIONS_FILE "relations.dat"
#define OFFSET_EXT ".offs"
#define ID2REL_EXT ".id2rel"
#define MAPPED_WAYS "mapped_ways.n2w"

struct WayElement
{
  vector<uint64_t> nodes;
  uint64_t m_wayOsmId;

  explicit WayElement(uint64_t osmId) : m_wayOsmId(osmId) {}

  bool IsValid() const { return !nodes.empty(); }

  uint64_t GetOtherEndPoint(uint64_t id) const
  {
    if (id == nodes.front())
      return nodes.back();

    ASSERT(id == nodes.back(), ());
    return nodes.front();
  }

  template <class ToDo>
  void ForEachPoint(ToDo & toDo) const
  {
    for_each(nodes.begin(), nodes.end(), ref(toDo));
  }

  template <class ToDo>
  void ForEachPointOrdered(uint64_t start, ToDo & toDo)
  {
    if (start == nodes.front())
      for_each(nodes.begin(), nodes.end(), ref(toDo));
    else
      for_each(nodes.rbegin(), nodes.rend(), ref(toDo));
  }

  template <class TArchive>
  void Write(TArchive & ar) const
  {
    ar << nodes;
  }

  template <class TArchive>
  void Read(TArchive & ar)
  {
    ar >> nodes;
  }

  string ToString() const
  {
    stringstream ss;
    ss << nodes.size() << " " << m_wayOsmId;
    return ss.str();
  }
};

class RelationElement
{
  using TMembers = vector<pair<uint64_t, string>>;

public:
  TMembers nodes;
  TMembers ways;
  map<string, string> tags;

public:
  bool IsValid() const { return !(nodes.empty() && ways.empty()); }

  string GetType() const
  {
    auto it = tags.find("type");
    return ((it != tags.end()) ? it->second : string());
  }

  bool FindWay(uint64_t id, string & role) const { return FindRoleImpl(ways, id, role); }
  bool FindNode(uint64_t id, string & role) const { return FindRoleImpl(nodes, id, role); }

  template <class ToDo>
  void ForEachWay(ToDo & toDo) const
  {
    for (size_t i = 0; i < ways.size(); ++i)
      toDo(ways[i].first, ways[i].second);
  }

  void Swap(RelationElement & rhs)
  {
    nodes.swap(rhs.nodes);
    ways.swap(rhs.ways);
    tags.swap(rhs.tags);
  }

  template <class TArchive>
  void Write(TArchive & ar) const
  {
    ar << nodes << ways << tags;
  }

  template <class TArchive>
  void Read(TArchive & ar)
  {
    ar >> nodes >> ways >> tags;
  }

  string ToString() const
  {
    stringstream ss;
    ss << nodes.size() << " " << ways.size() << " " << tags.size();
    return ss.str();
  }

protected:
  bool FindRoleImpl(TMembers const & container, uint64_t id, string & role) const
  {
    for (auto const & e : container)
    {
      if (e.first == id)
      {
        role = e.second;
        return true;
      }
    }
    return false;
  }
};
