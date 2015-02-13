#include "osm_source.hpp"

#include "osm_decl.hpp"
#include "data_cache_file.hpp"

#include "coastlines_generator.hpp"
#include "world_map_generator.hpp"
#include "feature_generator.hpp"
#include "polygonizer.hpp"

#include "point_storage.hpp"

#include "xml_element.hpp"

#include "first_pass_parser.hpp"
#include "osm_element.hpp"

#include "../defines.hpp"
#include "../indexer/mercator.hpp"
#include "../indexer/classificator.hpp"

#include "../coding/parse_xml.hpp"

#include "../3party/o5mreader/o5mreader.h"


#define DECODE_O5M_COORD(coord) (static_cast<double>(coord) / 1E+7)

namespace
{
  class SourceReader
  {
    string const &m_filename;
    FILE * m_file;

  public:
    SourceReader(string const & filename) : m_filename(filename)
    {
      if (m_filename.empty())
      {
        LOG(LINFO, ("Read OSM data from stdin..."));
        m_file = freopen(NULL, "rb", stdin);
      }
      else
      {
        LOG(LINFO, ("Read OSM data from", filename));
        m_file = fopen(filename.c_str(), "rb");
      }
    }

    ~SourceReader()
    {
      if (!m_filename.empty())
        fclose(m_file);
    }

    inline FILE * Handle() { return m_file; }

    uint64_t Read(char * buffer, uint64_t bufferSize)
    {
      return fread(buffer, sizeof(char), bufferSize, m_file);
    }
  };
}


namespace feature
{

  template <class TNodesHolder>
  class FileHolder : public cache::BaseFileHolder<TNodesHolder, cache::DataFileReader, FileReader>
  {
    typedef cache::DataFileReader reader_t;
    typedef cache::BaseFileHolder<TNodesHolder, reader_t, FileReader> base_type;

    typedef typename base_type::offset_map_t offset_map_t;

    typedef typename base_type::user_id_t user_id_t;

    template <class TElement, class ToDo> struct process_base
    {
    protected:
      reader_t & m_reader;
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
        return this->m_toDo(id, this->m_reader);
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
    }
  };
} // namespace feature

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
} // namespace data

namespace
{

  class MainFeaturesEmitter
  {
    typedef WorldMapGenerator<feature::FeaturesCollector> WorldGenerator;
    typedef CountryMapGenerator<feature::Polygonizer<feature::FeaturesCollector> > CountriesGenerator;
    unique_ptr<CountriesGenerator> m_countries;
    unique_ptr<WorldGenerator> m_world;

    unique_ptr<CoastlineFeaturesGenerator> m_coasts;
    unique_ptr<feature::FeaturesCollector> m_coastsHolder;

    string m_srcCoastsFile;

    template <class T1, class T2> class CombinedEmitter
    {
      T1 * m_p1;
      T2 * m_p2;
    public:
      CombinedEmitter(T1 * p1, T2 * p2) : m_p1(p1), m_p2(p2) {}
      void operator() (FeatureBuilder1 const & fb, uint64_t)
      {
        if (m_p1) (*m_p1)(fb);
        if (m_p2) (*m_p2)(fb);
      }
    };

    enum TypeIndex
    {
      NATURAL_COASTLINE,
      NATURAL_LAND,
      PLACE_ISLAND,
      PLACE_ISLET,

      TYPES_COUNT
    };
    uint32_t m_types[TYPES_COUNT];

    inline uint32_t Type(TypeIndex i) const { return m_types[i]; }

  public:
    MainFeaturesEmitter(feature::GenerateInfo const & info)
    {
      Classificator const & c = classif();

      char const * arr[][2] = {
        { "natural", "coastline" },
        { "natural", "land" },
        { "place", "island" },
        { "place", "islet" }
      };
      STATIC_ASSERT(ARRAY_SIZE(arr) == TYPES_COUNT);

      for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
        m_types[i] = c.GetTypeByPath(vector<string>(arr[i], arr[i] + 2));

      m_srcCoastsFile = info.m_tmpDir + WORLD_COASTS_FILE_NAME + info.m_datFileSuffix;

      CHECK(!info.m_makeCoasts || !info.m_createWorld,
            ("We can't do make_coasts and generate_world at the same time"));

      if (!info.m_makeCoasts)
      {
        m_countries.reset(new CountriesGenerator(info));

        if (info.m_emitCoasts)
        {
          m_coastsHolder.reset(new feature::FeaturesCollector(
                                                     info.m_datFilePrefix + WORLD_COASTS_FILE_NAME + info.m_datFileSuffix));
        }
      }
      else
      {
        // 4-10 - level range for cells
        // 20000 - max points count per feature
        m_coasts.reset(new CoastlineFeaturesGenerator(Type(NATURAL_COASTLINE), 4, 10, 20000));

        m_coastsHolder.reset(new feature::FeaturesCollector(m_srcCoastsFile));
      }

      if (info.m_createWorld)
        m_world.reset(new WorldGenerator(info));
    }

    void operator() (FeatureBuilder1 fb)
    {
      uint32_t const coastType = Type(NATURAL_COASTLINE);
      bool const hasCoast = fb.HasType(coastType);

      if (m_coasts)
      {
        if (hasCoast)
        {
          CHECK ( fb.GetGeomType() != feature::GEOM_POINT, () );

          // leave only coastline type
          fb.SetType(coastType);
          (*m_coasts)(fb);
        }
      }
      else
      {
        if (hasCoast)
        {
          fb.PopExactType(Type(NATURAL_LAND));
          fb.PopExactType(coastType);
        }
        else if ((fb.HasType(Type(PLACE_ISLAND)) || fb.HasType(Type(PLACE_ISLET))) &&
                 fb.GetGeomType() == feature::GEOM_AREA)
        {
          fb.AddType(Type(NATURAL_LAND));
        }

        if (fb.RemoveInvalidTypes())
        {
          if (m_world)
            (*m_world)(fb);

          if (m_countries)
            (*m_countries)(fb);
        }
      }
    }

    /// @return false if coasts are not merged and FLAG_fail_on_coasts is set
    bool Finish()
    {
      if (m_world)
        m_world->DoMerge();

      if (m_coasts)
      {
        // Check and stop if some coasts were not merged
        if (!m_coasts->Finish())
          return false;

        size_t const count = m_coasts->GetCellsCount();
        LOG(LINFO, ("Generating coastline polygons", count));

        for (size_t i = 0; i < count; ++i)
        {
          vector<FeatureBuilder1> vecFb;
          m_coasts->GetFeatures(i, vecFb);

          for (size_t j = 0; j < vecFb.size(); ++j)
            (*m_coastsHolder)(vecFb[j]);
        }
      }
      else if (m_coastsHolder)
      {
        CombinedEmitter<feature::FeaturesCollector, CountriesGenerator>
        emitter(m_coastsHolder.get(), m_countries.get());
        feature::ForEachFromDatRawFormat(m_srcCoastsFile, emitter);
      }
      return true;
    }
    
    inline void GetNames(vector<string> & names) const
    {
      if (m_countries)
        names = m_countries->Parent().Names();
      else
        names.clear();
    }
  };
} // namespace anonymous


template <typename HolderT>
void BuildIntermediateDataFromO5M(SourceReader & stream, HolderT & holder)
{
  O5mreader * reader;
  O5mreaderRet rc;

  if ((rc = o5mreader_open(&reader, stream.Handle())) != O5MREADER_RET_OK)
  {
    LOG(LCRITICAL, ("O5M Open error:", o5mreader_strerror(rc)));
    exit(1);  
  }

  O5mreaderDataset ds;
  O5mreaderIterateRet ret;
  while ((ret = o5mreader_iterateDataSet(reader, &ds)) == O5MREADER_ITERATE_RET_NEXT)
  {
    O5mreaderIterateRet ret2;
    switch (ds.type)
    {
      case O5MREADER_DS_NODE:
      {
        // Could do something with ds.id, ds.lon, ds.lat here, lon and lat are ints in 1E+7 * degree units
        // convert to mercator
        double const lat = MercatorBounds::LatToY(DECODE_O5M_COORD(ds.lat));
        double const lng = MercatorBounds::LonToX(DECODE_O5M_COORD(ds.lon));
        holder.AddNode(ds.id, lat, lng);
      } break;

      case O5MREADER_DS_WAY:
      {
        // store way
        WayElement way(ds.id);
        uint64_t nodeId = 0;
        while ((ret2 = o5mreader_iterateNds(reader, &nodeId)) == O5MREADER_ITERATE_RET_NEXT)
          way.nodes.push_back(nodeId);

        if (way.IsValid())
          holder.AddWay(ds.id, way);
      } break;

      case O5MREADER_DS_REL:
      {
        // store relation
        RelationElement relation;
        uint64_t refId = 0;
        uint8_t type = 0;
        char * role = nullptr;
        while ((ret2 = o5mreader_iterateRefs(reader, &refId, &type, &role)) == O5MREADER_ITERATE_RET_NEXT)
        {
          // Could do something with refId (way or node or rel id depends on type), type and role
          if (type == O5MREADER_DS_NODE)
            relation.nodes.emplace_back(make_pair(refId, string(role)));
          else if (type == O5MREADER_DS_WAY)
            relation.ways.emplace_back(make_pair(refId, string(role)));
          // we just ignore type == "relation"
        }

        char * key = nullptr;
        char * val = nullptr;
        while ((ret2 = o5mreader_iterateTags(reader, &key, &val)) == O5MREADER_ITERATE_RET_NEXT)
          relation.tags.emplace(make_pair(string(key), string(val)));

        if (relation.IsValid())
          holder.AddRelation(ds.id, relation);
      } break;
    }
  }
}

void BuildFeaturesFromO5M(SourceReader & stream, BaseOSMParser & parser)
{
  O5mreader * reader;
  O5mreaderRet rc;

  if ((rc = o5mreader_open(&reader, stream.Handle())) != O5MREADER_RET_OK)
  {
    LOG(LCRITICAL, ("O5M Open error:", o5mreader_strerror(rc)));
    exit(1);
  }

  O5mreaderDataset ds;
  O5mreaderIterateRet ret;
  while ((ret = o5mreader_iterateDataSet(reader, &ds)) == O5MREADER_ITERATE_RET_NEXT)
  {
    XMLElement p;
    p.id = ds.id;

    O5mreaderIterateRet ret2;
    char * key = nullptr;
    char * val = nullptr;
    switch (ds.type)
    {
      case O5MREADER_DS_NODE:
      {
        p.tagKey = XMLElement::ET_NODE;
        p.lat = DECODE_O5M_COORD(ds.lat);
        p.lng = DECODE_O5M_COORD(ds.lon);
      } break;

      case O5MREADER_DS_WAY:
      {
        p.tagKey = XMLElement::ET_WAY;
        uint64_t nodeId = 0;
        while ((ret2 = o5mreader_iterateNds(reader, &nodeId)) == O5MREADER_ITERATE_RET_NEXT)
          p.AddND(nodeId);
      } break;

      case O5MREADER_DS_REL:
      {
        p.tagKey = XMLElement::ET_RELATION;
        uint64_t refId = 0;
        uint8_t type = 0;
        char * role = nullptr;
        while ((ret2 = o5mreader_iterateRefs(reader, &refId, &type, &role)) == O5MREADER_ITERATE_RET_NEXT)
        {
          string strType;
          switch (type)
          {
            case O5MREADER_DS_NODE:
              strType = "node";
              break;
            case O5MREADER_DS_WAY:
              strType = "way";
              break;
            case O5MREADER_DS_REL:
              strType = "relation";
              break;

            default:
              break;
          }
          p.AddMEMBER(refId, strType, role);
        }
      } break;
    }

    while ((ret2 = o5mreader_iterateTags(reader, &key, &val)) == O5MREADER_ITERATE_RET_NEXT)
      p.AddKV(key, val);
    parser.EmitElement(&p);
  }
}


///////////////////////////////////////////////////////////////////////////////////////////////////
// Generate functions implementations.
///////////////////////////////////////////////////////////////////////////////////////////////////


template <class NodesHolderT>
bool GenerateFeaturesImpl(feature::GenerateInfo & info, string const &osmFileType, string const & osmFileName = string())
{
  try
  {
    NodesHolderT nodes(info.m_tmpDir + NODES_FILE);

    typedef feature::FileHolder<NodesHolderT> HolderT;
    HolderT holder(nodes, info.m_tmpDir);
    holder.LoadIndex();

    MainFeaturesEmitter bucketer(info);
    SecondPassParser<MainFeaturesEmitter, HolderT> parser(
                  bucketer, holder,
                  info.m_makeCoasts ? classif().GetCoastType() : 0,
                  info.m_addressFile);

    SourceReader reader(osmFileName);

    if (osmFileType == "xml")
    {
      ParseXMLSequence(reader, parser);
    }
    else if (osmFileType == "o5m")
    {
      BuildFeaturesFromO5M(reader, parser);
    }
    else
    {
      LOG(LERROR, ("Unknown source type:", osmFileType));
      return false;
    }

    parser.Finish();

    // Stop if coasts are not merged and FLAG_fail_on_coasts is set
    if (!bucketer.Finish())
      return false;

    bucketer.GetNames(info.m_bucketNames);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LERROR, ("Error with file ", e.what()));
    return false;
  }

  return true;
}

template <class TNodesHolder>
bool GenerateIntermediateDataImpl(string const & dir, string const &osmFileType, string const & osmFileName = string())
{
  try
  {
    TNodesHolder nodes(dir + NODES_FILE);
    typedef data::FileHolder<TNodesHolder> HolderT;
    HolderT holder(nodes, dir);

    SourceReader reader(osmFileName);

    LOG(LINFO, ("Data sorce format:", osmFileType));

    if (osmFileType == "xml")
    {
      FirstPassParser<HolderT> parser(holder);
      ParseXMLSequence(reader, parser);
    }
    else if (osmFileType == "o5m")
    {
      BuildIntermediateDataFromO5M(reader, holder);
    }
    else
    {
      LOG(LERROR, ("Unknown source type:", osmFileType));
      return false;
    }

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


bool GenerateFeatures(feature::GenerateInfo & info, string const & nodeStorage, string const &osmFileType, string const & osmFileName)
{
  if (nodeStorage == "raw")
    return GenerateFeaturesImpl<RawFileShortPointStorage<BasePointStorage::MODE_READ>>(info, osmFileType, osmFileName);
  else if (nodeStorage == "map")
    return GenerateFeaturesImpl<MapFileShortPointStorage<BasePointStorage::MODE_READ>>(info, osmFileType, osmFileName);
  else if (nodeStorage == "mem")
    return GenerateFeaturesImpl<RawMemShortPointStorage<BasePointStorage::MODE_READ>>(info, osmFileType, osmFileName);
  else
    CHECK(nodeStorage.empty(), ("Incorrect node_storage type:", nodeStorage));
  return false;
}


bool GenerateIntermediateData(string const & dir, string const & nodeStorage, string const &osmFileType, string const & osmFileName)
{
  if (nodeStorage == "raw")
    return GenerateIntermediateDataImpl<RawFileShortPointStorage<BasePointStorage::MODE_WRITE>>(dir, osmFileType, osmFileName);
  else if (nodeStorage == "map")
    return GenerateIntermediateDataImpl<MapFileShortPointStorage<BasePointStorage::MODE_WRITE>>(dir, osmFileType, osmFileName);
  else if (nodeStorage == "mem")
    return GenerateIntermediateDataImpl<RawMemShortPointStorage<BasePointStorage::MODE_WRITE>>(dir, osmFileType, osmFileName);
  else
    CHECK(nodeStorage.empty(), ("Incorrect node_storage type:", nodeStorage));
  return false;
}

