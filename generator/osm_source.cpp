#include "generator/coastlines_generator.hpp"
#include "generator/data_cache_file.hpp"
#include "generator/feature_generator.hpp"
#include "generator/first_pass_parser.hpp"
#include "generator/osm_decl.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_o5m_source.hpp"
#include "generator/osm_source.hpp"
#include "generator/point_storage.hpp"
#include "generator/polygonizer.hpp"
#include "generator/world_map_generator.hpp"
#include "generator/xml_element.hpp"

#include "indexer/classificator.hpp"
#include "indexer/mercator.hpp"

#include "coding/parse_xml.hpp"

#include "std/fstream.hpp"

#include "defines.hpp"

#define DECODE_O5M_COORD(coord) (static_cast<double>(coord) / 1E+7)

SourceReader::SourceReader()
: m_file(unique_ptr<istream,Deleter>(&cin, Deleter(false)))
{
  LOG_SHORT(LINFO, ("Reading OSM data from stdin"));
}

SourceReader::SourceReader(string const & filename)
{
  CHECK(!filename.empty() , ("Filename can't be empty"));
  m_file = unique_ptr<istream, Deleter>(new ifstream(filename), Deleter());
  CHECK(static_cast<ifstream *>(m_file.get())->is_open() , ("Can't open file:", filename));
  LOG_SHORT(LINFO, ("Reading OSM data from", filename));
}

SourceReader::SourceReader(istringstream & stream)
: m_file(unique_ptr<istream, Deleter>(&stream, Deleter(false)))
{
  LOG_SHORT(LINFO, ("Reading OSM data from memory"));
}

uint64_t SourceReader::Read(char * buffer, uint64_t bufferSize)
{
  m_file->read(buffer, bufferSize);
  return m_file->gcount();
}


namespace
{
template <class TNodesHolder>
class IntermediateDataReader
    : public cache::BaseFileHolder<TNodesHolder, cache::DataFileReader, FileReader>
{
  using TReader = cache::DataFileReader;
  using TBase = cache::BaseFileHolder<TNodesHolder, TReader, FileReader>;

  using typename TBase::TKey;
  using TBase::m_nodes;
  using TBase::m_nodeToRelations;
  using TBase::m_ways;
  using TBase::m_wayToRelations;
  using TBase::m_relations;

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
  IntermediateDataReader(TNodesHolder & holder, string const & dir) : TBase(holder, dir) {}

  bool GetNode(TKey id, double & lat, double & lng) { return m_nodes.GetPoint(id, lat, lng); }

  bool GetWay(TKey id, WayElement & e) { return m_ways.Read(id, e); }

  template <class ToDo>
  void ForEachRelationByWay(TKey id, ToDo && toDo)
  {
    RelationProcessor<ToDo> processor(m_relations, toDo);
    m_wayToRelations.ForEachByKey(id, processor);
  }

  template <class ToDo>
  void ForEachRelationByNodeCached(TKey id, ToDo && toDo)
  {
    CachedRelationProcessor<ToDo> processor(m_relations, toDo);
    m_nodeToRelations.ForEachByKey(id, processor);
  }

  template <class ToDo>
  void ForEachRelationByWayCached(TKey id, ToDo && toDo)
  {
    CachedRelationProcessor<ToDo> processor(m_relations, toDo);
    m_wayToRelations.ForEachByKey(id, processor);
  }

  void LoadIndex()
  {
    m_ways.LoadOffsets();
    m_relations.LoadOffsets();

    m_nodeToRelations.ReadAll();
    m_wayToRelations.ReadAll();
  }
};

template <class TNodesHolder>
class IntermediateDataWriter
    : public cache::BaseFileHolder<TNodesHolder, cache::DataFileWriter, FileWriter>
{
  using TBase = cache::BaseFileHolder<TNodesHolder, cache::DataFileWriter, FileWriter>;

  using typename TBase::TKey;
  using TBase::m_nodes;
  using TBase::m_nodeToRelations;
  using TBase::m_ways;
  using TBase::m_wayToRelations;
  using TBase::m_relations;

  template <class TIndex, class TContainer>
  static void AddToIndex(TIndex & index, TKey relationId, TContainer const & values)
  {
    for (auto const & v : values)
      index.Add(v.first, relationId);
  }

public:
  IntermediateDataWriter(TNodesHolder & nodes, string const & dir) : TBase(nodes, dir) {}

  void AddNode(TKey id, double lat, double lng) { m_nodes.AddPoint(id, lat, lng); }

  void AddWay(TKey id, WayElement const & e) { m_ways.Write(id, e); }

  void AddRelation(TKey id, RelationElement const & e)
  {
    string const & relationType = e.GetType();
    if (!(relationType == "multipolygon" || relationType == "route" || relationType == "boundary"))
      return;

    m_relations.Write(id, e);
    AddToIndex(m_nodeToRelations, id, e.nodes);
    AddToIndex(m_wayToRelations, id, e.ways);
  }

  void SaveIndex()
  {
    m_ways.SaveOffsets();
    m_relations.SaveOffsets();

    m_nodeToRelations.WriteAll();
    m_wayToRelations.WriteAll();
  }
};

class MainFeaturesEmitter
{
  using TWorldGenerator = WorldMapGenerator<feature::FeaturesCollector>;
  using TCountriesGenerator = CountryMapGenerator<feature::Polygonizer<feature::FeaturesCollector>>;
  unique_ptr<TCountriesGenerator> m_countries;
  unique_ptr<TWorldGenerator> m_world;

  unique_ptr<CoastlineFeaturesGenerator> m_coasts;
  unique_ptr<feature::FeaturesCollector> m_coastsHolder;

  string m_srcCoastsFile;
  bool m_failOnCoasts;

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
  : m_failOnCoasts(info.m_failOnCoasts)
  {
    Classificator const & c = classif();

    char const * arr[][2] = {
        {"natural", "coastline"}, {"natural", "land"}, {"place", "island"}, {"place", "islet"}};
    static_assert(ARRAY_SIZE(arr) == TYPES_COUNT, "");

    for (size_t i = 0; i < ARRAY_SIZE(arr); ++i)
      m_types[i] = c.GetTypeByPath({arr[i][0], arr[i][1]});

    m_srcCoastsFile = info.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, ".geom");

    CHECK(!info.m_makeCoasts || !info.m_createWorld,
          ("We can't do make_coasts and generate_world at the same time"));

    if (info.m_makeCoasts)
    {
      m_coasts.reset(new CoastlineFeaturesGenerator(Type(NATURAL_COASTLINE)));

      m_coastsHolder.reset(new feature::FeaturesAndRawGeometryCollector(
          m_srcCoastsFile,
          info.GetIntermediateFileName(WORLD_COASTS_FILE_NAME, RAW_GEOM_FILE_EXTENSION)));
      return;
    }

    if (info.m_emitCoasts)
      m_coastsHolder.reset(
          new feature::FeaturesCollector(info.GetTmpFileName(WORLD_COASTS_FILE_NAME)));

    if (info.m_splitByPolygons || !info.m_fileName.empty())
      m_countries.reset(new TCountriesGenerator(info));

    if (info.m_createWorld)
      m_world.reset(new TWorldGenerator(info));
  }

  void operator()(FeatureBuilder1 fb)
  {
    uint32_t const coastType = Type(NATURAL_COASTLINE);
    bool const hasCoast = fb.HasType(coastType);

    if (m_coasts)
    {
      if (hasCoast)
      {
        CHECK(fb.GetGeomType() != feature::GEOM_POINT, ());
        // leave only coastline type
        fb.SetType(coastType);
        (*m_coasts)(fb);
      }
      return;
    }

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

    if (!fb.RemoveInvalidTypes())
      return;

    if (m_world)
      (*m_world)(fb);

    if (m_countries)
      (*m_countries)(fb);
  }

  /// @return false if coasts are not merged and FLAG_fail_on_coasts is set
  bool Finish()
  {
    if (m_world)
      m_world->DoMerge();

    if (m_coasts)
    {
      // Check and stop if some coasts were not merged
      if (!m_coasts->Finish() && m_failOnCoasts)
        return false;

      LOG(LINFO, ("Generating coastline polygons"));

      size_t totalFeatures = 0;
      size_t totalPoints = 0;
      size_t totalPolygons = 0;

      vector<FeatureBuilder1> vecFb;
      m_coasts->GetFeatures(vecFb);

      for (auto & fb : vecFb)
      {
        (*m_coastsHolder)(fb);

        ++totalFeatures;
        totalPoints += fb.GetPointsCount();
        totalPolygons += fb.GetPolygonsCount();
      }
      LOG(LINFO, ("Total features:", totalFeatures, "total polygons:", totalPolygons,
                  "total points:", totalPoints));
    }
    else if (m_coastsHolder)
    {
      feature::ForEachFromDatRawFormat(m_srcCoastsFile, [this](FeatureBuilder1 const & fb, uint64_t)
      {
        if (m_coastsHolder)
          (*m_coastsHolder)(fb);
        if (m_countries)
          (*m_countries)(fb);
      });
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
}  // anonymous namespace

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
        // Could do something with em.id, em.lon, em.lat here
        // lon and lat are ints in 1E+7 * degree units
        // convert to mercator
        auto const pt = MercatorBounds::FromLatLon(DECODE_O5M_COORD(em.lat), DECODE_O5M_COORD(em.lon));
        holder.AddNode(em.id, pt.y, pt.x);
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
          // Could do something with member ref (way or node or rel id depends on type), type and
          // role
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
        p.lon = DECODE_O5M_COORD(em.lon);
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
          switch (member.type)
          {
            case TType::Node:
              p.AddMEMBER(member.ref, "node", member.role);
              break;
            case TType::Way:
              p.AddMEMBER(member.ref, "way", member.role);
              break;
            case TType::Relation:
              p.AddMEMBER(member.ref, "relation", member.role);
              break;

            default: break;
          }
        }
        break;
      }
      default: break;
    }

    for (auto const & tag : em.Tags())
      p.AddKV(tag.key, tag.value);

    parser.EmitElement(&p);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Generate functions implementations.
///////////////////////////////////////////////////////////////////////////////////////////////////

template <class TNodesHolder>
bool GenerateFeaturesImpl(feature::GenerateInfo & info)
{
  try
  {
    TNodesHolder nodes(info.GetIntermediateFileName(NODES_FILE, ""));

    using TDataCache = IntermediateDataReader<TNodesHolder>;
    TDataCache cache(nodes, info.m_intermediateDir);
    cache.LoadIndex();

    MainFeaturesEmitter bucketer(info);
    SecondPassParser<MainFeaturesEmitter, TDataCache> parser(
        bucketer, cache, info.m_makeCoasts ? classif().GetCoastType() : 0,
        info.GetAddressesFileName());

    SourceReader reader = info.m_osmFileName.empty() ? SourceReader() : SourceReader(info.m_osmFileName);
    switch (info.m_osmFileType)
    {
      case feature::GenerateInfo::OsmSourceType::XML:
        ParseXMLSequence(reader, parser);
        break;
      case feature::GenerateInfo::OsmSourceType::O5M:
        BuildFeaturesFromO5M(reader, parser);
        break;
    }

    LOG(LINFO, ("Processing", info.m_osmFileName, "done."));

    parser.Finish();

    // Stop if coasts are not merged and FLAG_fail_on_coasts is set
    if (!bucketer.Finish())
      return false;

    bucketer.GetNames(info.m_bucketNames);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LCRITICAL, ("Error with file ", e.what()));
  }

  return true;
}

template <class TNodesHolder>
bool GenerateIntermediateDataImpl(feature::GenerateInfo & info)
{
  try
  {
    TNodesHolder nodes(info.GetIntermediateFileName(NODES_FILE, ""));
    using TDataCache = IntermediateDataWriter<TNodesHolder>;
    TDataCache cache(nodes, info.m_intermediateDir);

    SourceReader reader = info.m_osmFileName.empty() ? SourceReader() : SourceReader(info.m_osmFileName);

    LOG(LINFO, ("Data source:", info.m_osmFileName));

    switch (info.m_osmFileType)
    {
      case feature::GenerateInfo::OsmSourceType::XML:
      {
        FirstPassParser<TDataCache> parser(cache);
        ParseXMLSequence(reader, parser);
        break;
      }
      case feature::GenerateInfo::OsmSourceType::O5M:
        BuildIntermediateDataFromO5M(reader, cache);
        break;
    }

    cache.SaveIndex();
    LOG(LINFO, ("Added points count = ", nodes.GetCount()));
  }
  catch (Writer::Exception const & e)
  {
    LOG(LCRITICAL, ("Error with file ", e.what()));
  }
  return true;
}

bool GenerateFeatures(feature::GenerateInfo & info)
{
  switch (info.m_nodeStorageType)
  {
    case feature::GenerateInfo::NodeStorageType::File:
      return GenerateFeaturesImpl<RawFileShortPointStorage<BasePointStorage::MODE_READ>>(info);
    case feature::GenerateInfo::NodeStorageType::Index:
      return GenerateFeaturesImpl<MapFileShortPointStorage<BasePointStorage::MODE_READ>>(info);
    case feature::GenerateInfo::NodeStorageType::Memory:
      return GenerateFeaturesImpl<RawMemShortPointStorage<BasePointStorage::MODE_READ>>(info);
  }
  return false;
}

bool GenerateIntermediateData(feature::GenerateInfo & info)
{
  switch (info.m_nodeStorageType)
  {
    case feature::GenerateInfo::NodeStorageType::File:
      return GenerateIntermediateDataImpl<RawFileShortPointStorage<BasePointStorage::MODE_WRITE>>(info);
    case feature::GenerateInfo::NodeStorageType::Index:
      return GenerateIntermediateDataImpl<MapFileShortPointStorage<BasePointStorage::MODE_WRITE>>(info);
    case feature::GenerateInfo::NodeStorageType::Memory:
      return GenerateIntermediateDataImpl<RawMemShortPointStorage<BasePointStorage::MODE_WRITE>>(info);
  }
  return false;
}
