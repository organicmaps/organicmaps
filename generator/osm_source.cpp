#include "generator/coastlines_generator.hpp"
#include "generator/data_cache_file.hpp"
#include "generator/feature_generator.hpp"
#include "generator/first_pass_parser.hpp"
#include "generator/osm_decl.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_source.hpp"
#include "generator/point_storage.hpp"
#include "generator/polygonizer.hpp"
#include "generator/source_reader.hpp"
#include "generator/world_map_generator.hpp"
#include "generator/xml_element.hpp"
#include "generator/osm_o5m_source.hpp"

#include "indexer/classificator.hpp"
#include "indexer/mercator.hpp"

#include "coding/parse_xml.hpp"

#include "defines.hpp"


#define DECODE_O5M_COORD(coord) (static_cast<double>(coord) / 1E+7)

namespace feature
{

  template <class TNodesHolder>
  class FileHolder : public cache::BaseFileHolder<TNodesHolder, cache::DataFileReader, FileReader>
  {
    using TReader = cache::DataFileReader;
    using TBase = cache::BaseFileHolder<TNodesHolder, TReader, FileReader>;

    using typename TBase::TKey;

    template <class TElement, class ToDo>
    struct ElementProcessorBase
    {
    protected:
      TReader & m_reader;
      ToDo & m_toDo;

    public:
      ElementProcessorBase(TReader & reader, ToDo & toDo) : m_reader(reader), m_toDo(toDo) {}

      bool operator()(uint64_t id)
      {
        TElement e;
        return m_reader.Read(id, e) ? m_toDo(id, e) : false;
      }
    };

    template <class ToDo>
    struct RelationProcessor : public ElementProcessorBase<RelationElement, ToDo>
    {
      using TBase = ElementProcessorBase<RelationElement, ToDo>;

      RelationProcessor(TReader & reader, ToDo & toDo) : TBase(reader, toDo) {}
    };

    template <class ToDo>
    struct CachedRelationProcessor : public RelationProcessor<ToDo>
    {
      using TBase = RelationProcessor<ToDo>;

      CachedRelationProcessor(TReader & rels, ToDo & toDo) : TBase(rels, toDo) {}
      bool operator()(uint64_t id) { return this->m_toDo(id, this->m_reader); }
    };

  public:
    FileHolder(TNodesHolder & holder, string const & dir) : TBase(holder, dir) {}

    bool GetNode(uint64_t id, double & lat, double & lng)
    {
      return this->m_nodes.GetPoint(id, lat, lng);
    }

    bool GetWay(TKey id, WayElement & e)
    {
      return this->m_ways.Read(id, e);
    }

    template <class ToDo>
    void ForEachRelationByWay(TKey id, ToDo && toDo)
    {
      RelationProcessor<ToDo> processor(this->m_relations, toDo);
      this->m_ways2rel.ForEachByKey(id, processor);
    }

    template <class ToDo>
    void ForEachRelationByNodeCached(TKey id, ToDo && toDo)
    {
      CachedRelationProcessor<ToDo> processor(this->m_relations, toDo);
      this->m_nodes2rel.ForEachByKey(id, processor);
    }

    template <class ToDo>
    void ForEachRelationByWayCached(TKey id, ToDo && toDo)
    {
      CachedRelationProcessor<ToDo> processor(this->m_relations, toDo);
      this->m_ways2rel.ForEachByKey(id, processor);
    }
    
    void LoadIndex()
    {
      this->m_ways.LoadOffsets();
      this->m_relations.LoadOffsets();
      
      this->m_nodes2rel.ReadAll();
      this->m_ways2rel.ReadAll();
    }
  };
} // namespace feature

namespace data
{

  template <class TNodesHolder>
  class FileHolder : public cache::BaseFileHolder<TNodesHolder, cache::DataFileWriter, FileWriter>
  {
    typedef cache::BaseFileHolder<TNodesHolder, cache::DataFileWriter, FileWriter> base_type;

    typedef typename base_type::TKey user_id_t;

    template <class TMap, class TVec>
    void add_id2rel_vector(TMap & rMap, user_id_t relid, TVec const & v)
    {
      for (size_t i = 0; i < v.size(); ++i)
        rMap.Write(v[i].first, relid);
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
      
      this->m_nodes2rel.Flush();
      this->m_ways2rel.Flush();
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
      static_assert(ARRAY_SIZE(arr) == TYPES_COUNT, "");

      for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
        m_types[i] = c.GetTypeByPath(vector<string>(arr[i], arr[i] + 2));

      m_srcCoastsFile = info.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, ".geom");

      CHECK(!info.m_makeCoasts || !info.m_createWorld,
            ("We can't do make_coasts and generate_world at the same time"));

      if (!info.m_makeCoasts)
      {
        m_countries.reset(new CountriesGenerator(info));

        if (info.m_emitCoasts)
        {
          m_coastsHolder.reset(new feature::FeaturesCollector(info.GetTmpFileName(WORLD_COASTS_FILE_NAME)));
        }
      }
      else
      {
        m_coasts.reset(new CoastlineFeaturesGenerator(Type(NATURAL_COASTLINE)));

        m_coastsHolder.reset(new feature::FeaturesAndRawGeometryCollector(
                               m_srcCoastsFile, info.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, ".rawdump")));
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

        LOG(LINFO, ("Generating coastline polygons"));

        size_t totalRegions = 0;
        size_t totalPoints = 0;
        size_t totalPolygons = 0;

        vector<FeatureBuilder1> vecFb;
        m_coasts->GetFeatures(vecFb);

        for (auto & fb : vecFb)
        {
          (*m_coastsHolder)(fb);
          ++totalRegions;
          totalPoints += fb.GetPointsCount();
          totalPolygons += fb.GetPolygonsCount();
        }
        LOG(LINFO, ("Total regions:", totalRegions, "total points:", totalPoints, "total polygons:",
                    totalPolygons));
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
  using TType = osm::O5MSource::EntityType;

  osm::O5MSource dataset([&stream](uint8_t * buffer, size_t size)
  {
   return stream.Read(reinterpret_cast<char *>(buffer), size);
  });

  for (auto const & em : dataset)
  {
    switch (em.type)
    {
      case TType::Node:
      {
        // Could do something with em.id, em.lon, em.lat here, lon and lat are ints in 1E+7 * degree units
        // convert to mercator
        double const lat = MercatorBounds::LatToY(DECODE_O5M_COORD(em.lat));
        double const lng = MercatorBounds::LonToX(DECODE_O5M_COORD(em.lon));
        holder.AddNode(em.id, lat, lng);
        break;
      }
      case TType::Way:
      {
        // store way
        WayElement way(em.id);
        for (uint64_t nd : em.Nodes())
          way.nodes.push_back(nd);

        if (way.IsValid())
          holder.AddWay(em.id, way);
        break;
      }
      case TType::Relation:
      {
        // store relation
        RelationElement relation;
        for (auto const & member : em.Members())
        {
          // Could do something with member ref (way or node or rel id depends on type), type and role
          if (member.type == TType::Node)
            relation.nodes.emplace_back(make_pair(member.ref, string(member.role)));
          else if (member.type == TType::Way)
            relation.ways.emplace_back(make_pair(member.ref, string(member.role)));
          // we just ignore type == "relation"
        }

        for (auto const & tag : em.Tags())
          relation.tags.emplace(make_pair(string(tag.key), string(tag.value)));

        if (relation.IsValid())
          holder.AddRelation(em.id, relation);

        break;
      }
      default:
        break;
    }
  }
}

void BuildFeaturesFromO5M(SourceReader & stream, BaseOSMParser & parser)
{
  using TType = osm::O5MSource::EntityType;

  osm::O5MSource dataset([&stream](uint8_t * buffer, size_t size)
  {
   return stream.Read(reinterpret_cast<char *>(buffer), size);
  });

  for (auto const & em : dataset)
  {
    XMLElement p;
    p.id = em.id;

    switch (em.type)
    {
      case TType::Node:
      {
        p.tagKey = XMLElement::ET_NODE;
        p.lat = DECODE_O5M_COORD(em.lat);
        p.lng = DECODE_O5M_COORD(em.lon);
        break;
      }
      case TType::Way:
      {
        p.tagKey = XMLElement::ET_WAY;
        for (uint64_t nd : em.Nodes())
          p.AddND(nd);
        break;
      }
      case TType::Relation:
      {
        p.tagKey = XMLElement::ET_RELATION;
        for (auto const & member : em.Members())
        {
          string strType;
          switch (member.type)
          {
            case TType::Node:
              strType = "node";
              break;
            case TType::Way:
              strType = "way";
              break;
            case TType::Relation:
              strType = "relation";
              break;

            default:
              break;
          }
          p.AddMEMBER(member.ref, strType, member.role);
        }
        break;
      }
      default:
        break;
    }

    for (auto const & tag : em.Tags())
      p.AddKV(tag.key, tag.value);

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
    NodesHolderT nodes(info.m_intermediateDir + NODES_FILE);

    typedef feature::FileHolder<NodesHolderT> HolderT;
    HolderT holder(nodes, info.m_intermediateDir);
    holder.LoadIndex();

    MainFeaturesEmitter bucketer(info);
    SecondPassParser<MainFeaturesEmitter, HolderT> parser(
                  bucketer, holder,
                  info.m_makeCoasts ? classif().GetCoastType() : 0,
                  info.GetAddressesFileName());

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

    LOG(LINFO, ("Processing", osmFileType, "file done."));

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

