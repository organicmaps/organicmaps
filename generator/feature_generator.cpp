#include "feature_generator.hpp"
#include "data_cache_file.hpp"
#include "osm_element.hpp"
#include "polygonizer.hpp"
#include "osm_decl.hpp"

#include "../defines.hpp"

#include "../indexer/data_header.hpp"
#include "../indexer/mercator.hpp"
#include "../indexer/cell_id.hpp"

#include "../coding/varint.hpp"
#include "../coding/mmap_reader.hpp"

#include "../base/assert.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/bind.hpp"
#include "../std/unordered_map.hpp"
#include "../std/target_os.hpp"

namespace feature
{

template <class TNodesHolder>
class FileHolder : public cache::BaseFileHolder<TNodesHolder, cache::DataFileReader, FileReader>
{
  typedef cache::DataFileReader reader_t;
  typedef cache::BaseFileHolder<TNodesHolder, reader_t, FileReader> base_type;

  typedef typename base_type::offset_map_t offset_map_t;
  typedef typename base_type::ways_map_t ways_map_t;

  typedef typename base_type::user_id_t user_id_t;

  template <class TElement, class ToDo> struct process_base
  {
    reader_t & m_reader;
  protected:
    ToDo & m_toDo;
  public:
    process_base(reader_t & reader, ToDo & toDo) : m_reader(reader), m_toDo(toDo) {}

    bool operator() (uint64_t id)
    {
      TElement e;
      if (m_reader.Read(id, e))
        return m_toDo(id, e);
      return false;
    }
  };

  template <class ToDo> struct process_relation : public process_base<RelationElement, ToDo>
  {
    typedef process_base<RelationElement, ToDo> base_type;
  public:
    process_relation(reader_t & reader, ToDo & toDo) : base_type(reader, toDo) {}
  };

  template <class ToDo> struct process_relation_cached : public process_relation<ToDo>
  {
    typedef process_relation<ToDo> base_type;

  public:
    process_relation_cached(reader_t & rels, ToDo & toDo)
      : base_type(rels, toDo) {}

    bool operator() (uint64_t id)
    {
      switch (this->m_toDo(id))
      {
      case 1:   return true;
      case -1:  return false;
      default:  return base_type::operator()(id);
      }
    }
  };

public:
  FileHolder(TNodesHolder & holder, string const & dir) : base_type(holder, dir) {}

  bool GetNode(uint64_t id, double & lat, double & lng)
  {
    return this->m_nodes.GetPoint(id, lat, lng);
  }

  bool GetWay(user_id_t id, WayElement & e)
  {
    return this->m_ways.Read(id, e);
  }

  bool GetNextWay(user_id_t & prevWay, user_id_t node, WayElement & e)
  {
    typedef typename ways_map_t::iter_t iter_t;
    pair<iter_t, iter_t> range = this->m_mappedWays.GetRange(node);
    for (; range.first != range.second; ++range.first)
    {
      cache::MappedWay const & w = range.first->second;
      if (w.GetType() != cache::MappedWay::coast_opposite && w.GetId() != prevWay)
      {
        this->m_ways.Read(w.GetId(), e);
        prevWay = w.GetId();
        return true;
      }
    }
    return false;
  }

  template <class ToDo> void ForEachRelationByWay(user_id_t id, ToDo & toDo)
  {
    process_relation<ToDo> processor(this->m_relations, toDo);
    this->m_ways2rel.for_each_ret(id, processor);
  }

  template <class ToDo> void ForEachRelationByNodeCached(user_id_t id, ToDo & toDo)
  {
    process_relation_cached<ToDo> processor(this->m_relations, toDo);
    this->m_nodes2rel.for_each_ret(id, processor);
  }

  template <class ToDo> void ForEachRelationByWayCached(user_id_t id, ToDo & toDo)
  {
    process_relation_cached<ToDo> processor(this->m_relations, toDo);
    this->m_ways2rel.for_each_ret(id, processor);
  }

  void LoadIndex()
  {
    this->m_ways.LoadOffsets();
    this->m_relations.LoadOffsets();

    this->m_nodes2rel.read_to_memory();
    this->m_ways2rel.read_to_memory();
    this->m_mappedWays.read_to_memory();
  }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeaturesCollector implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

FeaturesCollector::FeaturesCollector(string const & fName)
: m_datFile(fName)
{
}

FeaturesCollector::FeaturesCollector(string const & bucket,
                                     FeaturesCollector::InitDataType const & prefix)
: m_datFile(prefix.first + bucket + prefix.second)
{
}

uint32_t FeaturesCollector::GetFileSize(FileWriter const & f)
{
  // .dat file should be less than 4Gb
  uint64_t const pos = f.Pos();
  uint32_t const ret = static_cast<uint32_t>(pos);

  CHECK_EQUAL(static_cast<uint64_t>(ret), pos, ("Feature offset is out of 32bit boundary!"));
  return ret;
}

void FeaturesCollector::WriteFeatureBase(vector<char> const & bytes, FeatureBuilder1 const & fb)
{
  size_t const sz = bytes.size();
  CHECK ( sz != 0, ("Empty feature not allowed here!") );

  if (sz > 0)
  {
    WriteVarUint(m_datFile, sz);
    m_datFile.Write(&bytes[0], sz);

    m_bounds.Add(fb.GetLimitRect());
  }
}

void FeaturesCollector::operator() (FeatureBuilder1 const & fb)
{
  (void)GetFileSize(m_datFile);

  FeatureBuilder1::buffer_t bytes;
  fb.Serialize(bytes);
  WriteFeatureBase(bytes, fb);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Generate functions implementations.
///////////////////////////////////////////////////////////////////////////////////////////////////

class points_in_file
{
#ifdef OMIM_OS_WINDOWS
  FileReader m_file;
#else
  MmapReader m_file;
#endif

public:
  points_in_file(string const & name) : m_file(name) {}

  bool GetPoint(uint64_t id, double & lat, double & lng) const
  {
#ifdef OMIM_OS_WINDOWS
    LatLon ll;
    m_file.Read(id * sizeof(ll), &ll, sizeof(ll));
#else
    LatLon const & ll = *reinterpret_cast<LatLon const *>(m_file.Data() + id * sizeof(ll));
#endif
    // assume that valid coordinate is not (0, 0)
    if (ll.lat != 0.0 || ll.lon != 0.0)
    {
      lat = ll.lat;
      lng = ll.lon;
      return true;
    }
    else
    {
      LOG(LERROR, ("Node with id = ", id, " not found!"));
      return false;
    }
  }
};

class points_in_map
{
  typedef unordered_map<uint64_t, pair<double, double> > cont_t;
  typedef cont_t::const_iterator iter_t;
  cont_t m_map;

  static bool equal_coord(double d1, double d2)
  {
    return ::fabs(d1 - d2) < 1.0E-8;
  }

public:
  points_in_map(string const & name)
  {
    FileReader reader(name);
    uint64_t const count = reader.Size();

    uint64_t pos = 0;
    while (pos < count)
    {
      LatLonPos ll;
      reader.Read(pos, &ll, sizeof(ll));

      pair<iter_t, bool> ret = m_map.insert(make_pair(ll.pos, make_pair(ll.lat, ll.lon)));
      if (ret.second == true)
      {
#ifdef DEBUG
        pair<double, double> const & c = ret.first->second;
        ASSERT ( equal_coord(c.first, ll.lat), () );
        ASSERT ( equal_coord(c.second, ll.lon), () );
#endif
      }

      pos += sizeof(ll);
    }
  }

  bool GetPoint(uint64_t id, double & lat, double & lng) const
  {
    iter_t i = m_map.find(id);
    if (i != m_map.end())
    {
      lat = i->second.first;
      lng = i->second.second;
      return true;
    }
    return false;
  }
};

template <class TNodesHolder, template <class, class> class TParser>
bool GenerateImpl(GenerateInfo & info)
{
  try
  {
    TNodesHolder nodes(info.m_tmpDir + NODES_FILE);

    typedef FileHolder<TNodesHolder> holder_t;
    holder_t holder(nodes, info.m_tmpDir);

    holder.LoadIndex();

    typedef Polygonizer<FeaturesCollector> PolygonizerT;
    // prefix is data dir
    PolygonizerT bucketer(info);
    TParser<PolygonizerT, holder_t> parser(bucketer, holder);
    ParseXMLFromStdIn(parser);
    info.m_bucketNames = bucketer.Names();
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error with file ", e.what()));
    return false;
  }

  return true;
}

bool GenerateFeatures(GenerateInfo & info, bool lightNodes)
{
  if (lightNodes)
    return GenerateImpl<points_in_map, SecondPassParserUsual>(info);
  else
    return GenerateImpl<points_in_file, SecondPassParserUsual>(info);
}

}
