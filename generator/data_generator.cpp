#include "data_generator.hpp"
#include "data_cache_file.hpp"
#include "first_pass_parser.hpp"
#include "osm_decl.hpp"

#include "../base/std_serialization.hpp"
#include "../base/logging.hpp"

#include "../std/bind.hpp"
#include "point_storage.hpp"

namespace data
{

template <class TNodesHolder>
class FileHolder : public cache::BaseFileHolder<TNodesHolder, cache::DataFileWriter, FileWriter>
{
  typedef cache::BaseFileHolder<TNodesHolder, cache::DataFileWriter, FileWriter> base_type;

  typedef typename base_type::user_id_t user_id_t;

  template <class TMap, class TVec>
  void add_id2rel_vector(TMap & rMap, user_id_t relid, TVec const & v)
  {
    for (size_t i = 0; i < v.size(); ++i)
      rMap.write(v[i].first, relid);
  }

public:
  FileHolder(TNodesHolder & nodes, string const & dir) : base_type(nodes, dir) {}

  void AddNode(uint64_t id, double lat, double lng)
  {
    this->m_nodes.AddPoint(id, lat, lng);
  }

  void AddWay(user_id_t id, WayElement const & e)
  {
    this->m_ways.Write(id, e);
  }

  void AddRelation(user_id_t id, RelationElement const & e)
  {
    const string relationType = e.GetType();
    if (relationType == "multipolygon" || relationType == "route" || relationType == "boundary")
    {
      this->m_relations.Write(id, e);

      add_id2rel_vector(this->m_nodes2rel, id, e.nodes);
      add_id2rel_vector(this->m_ways2rel, id, e.ways);
    }
  }

  void SaveIndex()
  {
    this->m_ways.SaveOffsets();
    this->m_relations.SaveOffsets();

    this->m_nodes2rel.flush_to_file();
    this->m_ways2rel.flush_to_file();
  }
};

template <class TNodesHolder>
bool GenerateImpl(string const & dir, string const & osmFileName = string())
{
  try
  {
    TNodesHolder nodes(dir + NODES_FILE);
    typedef FileHolder<TNodesHolder> holder_t;
    holder_t holder(nodes, dir);

    FirstPassParser<holder_t> parser(holder);
    if (osmFileName.empty())
      ParseXMLFromStdIn(parser);
    else
      ParseXMLFromFile(parser, osmFileName);

    LOG(LINFO, ("Added points count = ", nodes.GetCount()));

    holder.SaveIndex();
  }
  catch (Writer::Exception const & e)
  {
    LOG(LERROR, ("Error with file ", e.what()));
    return false;
  }

  return true;
}

bool GenerateToFile(string const & dir, string const & nodeStorage, string const & osmFileName)
{
  if (nodeStorage == "raw")
    return GenerateImpl<RawFilePointStorage<BasePointStorage::MODE_WRITE>>(dir, osmFileName);
  else if (nodeStorage == "map")
    return GenerateImpl<MapFileShortPointStorage<BasePointStorage::MODE_WRITE>>(dir, osmFileName);
  else if (nodeStorage == "sqlite")
    return GenerateImpl<SQLitePointStorage<BasePointStorage::MODE_WRITE>>(dir, osmFileName);
  else if (nodeStorage == "mem")
    return GenerateImpl<RawFileShortPointStorage<BasePointStorage::MODE_WRITE>>(dir, osmFileName);
  else
    CHECK(nodeStorage.empty(), ("Incorrect node_storage type:", nodeStorage));
  return false;
}

}
