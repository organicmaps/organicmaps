#pragma once

#include "../base/std_serialization.hpp"
#include "../base/assert.hpp"

#include "../std/utility.hpp"
#include "../std/vector.hpp"
#include "../std/string.hpp"
#include "../std/algorithm.hpp"
#include "../std/bind.hpp"


/// Used to store all world nodes inside temporary index file.
/// To find node by id, just calculate offset inside index file:
/// offset_in_file = sizeof(LatLon) * node_ID
struct LatLon
{
  double lat;
  double lon;
};
STATIC_ASSERT(sizeof(LatLon) == 16);

struct LatLonPos
{
  uint64_t pos;
  double lat;
  double lon;
};
STATIC_ASSERT(sizeof(LatLonPos) == 24);

#define NODES_FILE "nodes.dat"
#define WAYS_FILE "ways.dat"
#define RELATIONS_FILE "relations.dat"
#define OFFSET_EXT ".offs"
#define ID2REL_EXT ".id2rel"
#define MAPPED_WAYS "mapped_ways.n2w"


namespace feature
{
  /// @name Need to unite features.
  //@{
  /// @param[in]  k, v  Key and Value from relation tags.
  bool NeedUnite(string const & k, string const & v);
  /// @param[in]  type  Type from feature.
  bool NeedUnite(uint32_t type);
  //@}
};

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
    for_each(nodes.begin(), nodes.end(), bind<void>(ref(toDo), _1));
  }

  template <class ToDo> void ForEachPointOrdered(uint64_t start, ToDo & toDo)
  {
    if (start == nodes.front())
      for_each(nodes.begin(), nodes.end(), bind<void>(ref(toDo), _1));
    else
      for_each(nodes.rbegin(), nodes.rend(), bind<void>(ref(toDo), _1));
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

  template <class ToDo> void ForEachWay(ToDo & toDo) const
  {
    for (size_t i = 0; i < ways.size(); ++i)
      toDo(ways[i].first, ways[i].second);
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
