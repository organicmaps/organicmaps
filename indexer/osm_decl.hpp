#pragma once

#include "std_serialization.hpp"

#include "../std/utility.hpp"
#include "../std/vector.hpp"
#include "../std/string.hpp"


/// Used to store all world nodes inside temporary index file.
/// To find node by id, just calculate offset inside index file:
/// offset_in_file = sizeof(LatLon) * node_ID
#pragma pack (push, 1)
struct LatLon
{
  double lat;
  double lon;
};

struct LatLonPos
{
  uint64_t pos;
  double lat;
  double lon;
};
#pragma pack (pop)


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

  template <class ToDo> void ForEachPoint(ToDo & toDo) const
  {
    for (size_t i = 0; i < nodes.size(); ++i)
      toDo(nodes[i]);
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
