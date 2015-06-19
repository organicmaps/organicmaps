#pragma once

#include "base/std_serialization.hpp"
#include "base/assert.hpp"

#include "std/utility.hpp"
#include "std/vector.hpp"
#include "std/string.hpp"
#include "std/algorithm.hpp"
#include "std/bind.hpp"


#define NODES_FILE "nodes.dat"
#define WAYS_FILE "ways.dat"
#define RELATIONS_FILE "relations.dat"
#define OFFSET_EXT ".offs"
#define ID2REL_EXT ".id2rel"
#define MAPPED_WAYS "mapped_ways.n2w"


class progress_policy
{
  size_t m_count;
  size_t m_factor;

public:
  size_t GetCount() const { return m_count; }

  void Begin(string const &, size_t factor);
  void Inc(size_t i = 1);
  void End();
};

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
    else
    {
      ASSERT ( id == nodes.back(), () );
      return nodes.front();
    }
  }

  template <class ToDo> void ForEachPoint(ToDo & toDo) const
  {
    for_each(nodes.begin(), nodes.end(), ref(toDo));
  }

  template <class ToDo> void ForEachPointOrdered(uint64_t start, ToDo & toDo)
  {
    if (start == nodes.front())
      for_each(nodes.begin(), nodes.end(), ref(toDo));
    else
      for_each(nodes.rbegin(), nodes.rend(), ref(toDo));
  }
};

struct RelationElement
{
  typedef vector<pair<uint64_t, string> > ref_vec_t;
  ref_vec_t nodes, ways;
  map<string, string> tags;

  bool IsValid() const { return !(nodes.empty() && ways.empty()); }

  string GetType() const;
  bool FindWay(uint64_t id, string & role) const;
  bool FindNode(uint64_t id, string & role) const;

  template <class ToDo> void ForEachWay(ToDo & toDo) const
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
};

template <class TArchive> TArchive & operator << (TArchive & ar, WayElement const & e)
{
  ar << e.nodes;
  return ar;
}

template <class TArchive> TArchive & operator >> (TArchive & ar, WayElement & e)
{
  ar >> e.nodes;
  return ar;
}

template <class TArchive> TArchive & operator << (TArchive & ar, RelationElement const & e)
{
  ar << e.nodes << e.ways << e.tags;
  return ar;
}

template <class TArchive> TArchive & operator >> (TArchive & ar, RelationElement & e)
{
  ar >> e.nodes >> e.ways >> e.tags;
  return ar;
}
