#include "feature_generator.hpp"
#include "feature_bucketer.hpp"
#include "data_cache_file.hpp"
#include "osm_element.hpp"

#include "../../indexer/data_header.hpp"
#include "../../indexer/osm_decl.hpp"
#include "../../indexer/data_header_reader.hpp"
#include "../../indexer/mercator.hpp"
#include "../../indexer/cell_id.hpp"

#include "../../coding/varint.hpp"

#include "../../base/assert.hpp"
#include "../../base/logging.hpp"
#include "../../base/stl_add.hpp"

#include "../../std/bind.hpp"
#include "../../std/unordered_map.hpp"


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
      if (w.m_type != cache::MappedWay::coast_opposite && w.m_id != prevWay)
      {
        this->m_ways.Read(w.m_id, e);
        prevWay = w.m_id;
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

void FeaturesCollector::Init()
{
  // write empty stub, will be updated in Finish()
  WriteDataHeader(m_datFile, feature::DataHeader());
}

FeaturesCollector::FeaturesCollector(string const & fName)
: m_datFile(fName), m_geoFile(fName + ".geom"), m_trgFile(fName + ".trg")
{
  Init();
}

FeaturesCollector::FeaturesCollector(string const & bucket,
                                     FeaturesCollector::InitDataType const & prefix)
: m_datFile(prefix.first + bucket + prefix.second),
  m_geoFile(prefix.first + bucket + prefix.second + ".geom"),
  m_trgFile(prefix.first + bucket + prefix.second + ".trg")
{
  Init();
}

void FeaturesCollector::FilePreCondition(FileWriter const & f)
{
  // .dat file should be less than 4Gb
  uint64_t const pos = f.Pos();
  CHECK_EQUAL ( static_cast<uint64_t>(static_cast<uint32_t>(pos)), pos,
    ("Feature offset is out of 32bit boundary!") );
}

void FeaturesCollector::WriteBuffer(FileWriter & f, vector<char> const & bytes)
{
  if (!bytes.empty())
    f.Write(&bytes[0], bytes.size());
}

void FeaturesCollector::WriteFeatureBase(vector<char> const & bytes, FeatureBuilderGeom const & fb)
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

void FeaturesCollector::operator() (FeatureBuilderGeom const & fb)
{
  FilePreCondition(m_datFile);

  FeatureBuilderGeom::buffers_holder_t bytes;
  fb.Serialize(bytes);
  WriteFeatureBase(bytes, fb);
}

void FeaturesCollector::operator() (FeatureBuilderGeomRef const & fb)
{
  FilePreCondition(m_datFile);
  FilePreCondition(m_geoFile);
  FilePreCondition(m_trgFile);

  FeatureBuilderGeomRef::buffers_holder_t buffers;
  buffers.m_lineOffset = static_cast<uint32_t>(m_geoFile.Pos());
  buffers.m_trgOffset = static_cast<uint32_t>(m_trgFile.Pos());

  fb.Serialize(buffers);
  WriteFeatureBase(buffers.m_buffers[0], fb);

  WriteBuffer(m_geoFile, buffers.m_buffers[1]);
  WriteBuffer(m_trgFile, buffers.m_buffers[2]);
}

FeaturesCollector::~FeaturesCollector()
{
  // rewrite map information with actual data
  m_datFile.Seek(0);
  feature::DataHeader header;
  header.SetBounds(m_bounds);
  WriteDataHeader(m_datFile, header);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Generate functions implementations.
///////////////////////////////////////////////////////////////////////////////////////////////////

class points_in_file
{
  FileReader m_file;

public:
  points_in_file(string const & name) : m_file(name) {}

  bool GetPoint(uint64_t id, double & lat, double & lng) const
  {
    LatLon ll;
    m_file.Read(id * sizeof(ll), &ll, sizeof(ll));

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
  CHECK_GREATER_OR_EQUAL(info.cellBucketingLevel, 0, ());
  CHECK_LESS(info.cellBucketingLevel, 10, ());

  try
  {
    TNodesHolder nodes(info.dir + NODES_FILE);

    typedef FileHolder<TNodesHolder> holder_t;
    holder_t holder(nodes, info.dir);

    holder.LoadIndex();

    FeaturesCollector::InitDataType collectorInitData(info.datFilePrefix, info.datFileSuffix);

    typedef CellFeatureBucketer<FeaturesCollector, SimpleFeatureClipper, MercatorBounds, RectId>
        FeatureBucketerType;
    FeatureBucketerType bucketer(info.cellBucketingLevel, collectorInitData, info.m_maxScaleForWorldFeatures);
    {
      TParser<FeatureBucketerType, holder_t> parser(bucketer, holder);
      ParseXMLFromStdIn(parser);
    }
    bucketer.GetBucketNames(MakeBackInsertFunctor(info.bucketNames));
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

bool GenerateCoastlines(GenerateInfo & info, bool lightNodes)
{
  if (lightNodes)
    return GenerateImpl<points_in_map, SecondPassParserJoin>(info);
  else
    return GenerateImpl<points_in_file, SecondPassParserJoin>(info);
}

}
