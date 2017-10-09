#include "generator/osm_source.hpp"

#include "generator/cities_boundaries_builder.hpp"
#include "generator/coastlines_generator.hpp"
#include "generator/feature_generator.hpp"
#include "generator/intermediate_data.hpp"
#include "generator/intermediate_elements.hpp"
#include "generator/osm_element.hpp"
#include "generator/osm_o5m_source.hpp"
#include "generator/osm_translator.hpp"
#include "generator/osm_xml_source.hpp"
#include "generator/polygonizer.hpp"
#include "generator/tag_admixer.hpp"
#include "generator/towns_dumper.hpp"
#include "generator/world_map_generator.hpp"

#include "generator/booking_dataset.hpp"
#include "generator/opentable_dataset.hpp"
#include "generator/viator_dataset.hpp"

#include "indexer/city_boundary.hpp"
#include "indexer/classificator.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"
#include "geometry/tree4d.hpp"

#include "base/stl_helpers.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/parse_xml.hpp"

#include <memory>

#include "defines.hpp"

using namespace std;

SourceReader::SourceReader()
: m_file(unique_ptr<istream, Deleter>(&cin, Deleter(false)))
{
  LOG_SHORT(LINFO, ("Reading OSM data from stdin"));
}

SourceReader::SourceReader(string const & filename)
{
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
template <class TNodesHolder, cache::EMode TMode>
class IntermediateData
{
  using TReader = cache::OSMElementCache<TMode>;

  using TFile = typename conditional<TMode == cache::EMode::Write, FileWriter, FileReader>::type;

  using TKey = uint64_t;
  static_assert(is_integral<TKey>::value, "TKey is not integral type");

  using TIndex = cache::detail::IndexFile<TFile, TKey>;

  TNodesHolder & m_nodes;

  TReader m_ways;
  TReader m_relations;

  TIndex m_nodeToRelations;
  TIndex m_wayToRelations;

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

  template <class TIndex, class TContainer>
  static void AddToIndex(TIndex & index, TKey relationId, TContainer const & values)
  {
    for (auto const & v : values)
      index.Add(v.first, relationId);
  }

public:
  IntermediateData(TNodesHolder & nodes, feature::GenerateInfo & info)
  : m_nodes(nodes)
  , m_ways(info.GetIntermediateFileName(WAYS_FILE, ""), info.m_preloadCache)
  , m_relations(info.GetIntermediateFileName(RELATIONS_FILE, ""), info.m_preloadCache)
  , m_nodeToRelations(info.GetIntermediateFileName(NODES_FILE, ID2REL_EXT))
  , m_wayToRelations(info.GetIntermediateFileName(WAYS_FILE,ID2REL_EXT))
  {
  }

  void AddNode(TKey id, double lat, double lng) { m_nodes.AddPoint(id, lat, lng); }
  bool GetNode(TKey id, double & lat, double & lng) { return m_nodes.GetPoint(id, lat, lng); }

  void AddWay(TKey id, WayElement const & e) { m_ways.Write(id, e); }
  bool GetWay(TKey id, WayElement & e) { return m_ways.Read(id, e); }

  void AddRelation(TKey id, RelationElement const & e)
  {
    string const & relationType = e.GetType();
    if (!(relationType == "multipolygon" || relationType == "route" || relationType == "boundary" ||
          relationType == "associatedStreet" || relationType == "building" ||
          relationType == "restriction"))
    {
      return;
    }

    m_relations.Write(id, e);
    AddToIndex(m_nodeToRelations, id, e.nodes);
    AddToIndex(m_wayToRelations, id, e.ways);
  }

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

  void SaveIndex()
  {
    m_ways.SaveOffsets();
    m_relations.SaveOffsets();

    m_nodeToRelations.WriteAll();
    m_wayToRelations.WriteAll();
  }

  void LoadIndex()
  {
    m_ways.LoadOffsets();
    m_relations.LoadOffsets();

    m_nodeToRelations.ReadAll();
    m_wayToRelations.ReadAll();
  }
};

/// Used to make a "good" node for a highway graph with OSRM for low zooms.
class Place
{
public:
  Place(FeatureBuilder1 const & ft, uint32_t type) : m_ft(ft), m_pt(ft.GetKeyPoint()), m_type(type)
  {
    using namespace ftypes;

    switch (IsLocalityChecker::Instance().GetType(m_type))
    {
    case COUNTRY: m_thresholdM = 300000.0; break;
    case STATE: m_thresholdM = 100000.0; break;
    case CITY: m_thresholdM = 30000.0; break;
    case TOWN: m_thresholdM = 20000.0; break;
    case VILLAGE: m_thresholdM = 10000.0; break;
    default: m_thresholdM = 10000.0; break;
    }
  }

  FeatureBuilder1 const & GetFeature() const { return m_ft; }

  m2::RectD GetLimitRect() const
  {
    return MercatorBounds::RectByCenterXYAndSizeInMeters(m_pt, m_thresholdM);
  }

  bool IsEqual(Place const & r) const
  {
    return (AreTypesEqual(m_type, r.m_type) &&
            m_ft.GetName() == r.m_ft.GetName() &&
            (IsPoint() || r.IsPoint()) &&
            MercatorBounds::DistanceOnEarth(m_pt, r.m_pt) < m_thresholdM);
  }

  /// Check whether we need to replace place @r with place @this.
  bool IsBetterThan(Place const & r) const
  {
    // Check ranks.
    uint8_t const r1 = m_ft.GetRank();
    uint8_t const r2 = r.m_ft.GetRank();
    if (r1 != r2)
      return r1 > r2;

    // Check types length.
    // ("place-city-capital-2" is better than "place-city").
    uint8_t const l1 = ftype::GetLevel(m_type);
    uint8_t const l2 = ftype::GetLevel(r.m_type);
    if (l1 != l2)
      return l1 > l2;

    // Assume that area places has better priority than point places at the very end ...
    /// @todo It was usefull when place=XXX type has any area fill style.
    /// Need to review priority logic here (leave the native osm label).
    return !IsPoint() && r.IsPoint();
  }

private:
  bool IsPoint() const { return (m_ft.GetGeomType() == feature::GEOM_POINT); }

  static bool AreTypesEqual(uint32_t t1, uint32_t t2)
  {
    // Use 2-arity places comparison for filtering.
    // ("place-city-capital-2" is equal to "place-city")
    ftype::TruncValue(t1, 2);
    ftype::TruncValue(t2, 2);
    return (t1 == t2);
  }

  FeatureBuilder1 m_ft;
  m2::PointD m_pt;
  uint32_t m_type;
  double m_thresholdM;
};

class MainFeaturesEmitter : public EmitterBase
{
  using TWorldGenerator = WorldMapGenerator<feature::FeaturesCollector>;
  using TCountriesGenerator = CountryMapGenerator<feature::Polygonizer<feature::FeaturesCollector>>;

  unique_ptr<TCountriesGenerator> m_countries;
  unique_ptr<TWorldGenerator> m_world;

  unique_ptr<CoastlineFeaturesGenerator> m_coasts;
  unique_ptr<feature::FeaturesCollector> m_coastsHolder;

  string const m_skippedElementsPath;
  ostringstream m_skippedElements;

  string m_srcCoastsFile;
  bool m_failOnCoasts;

  generator::BookingDataset m_bookingDataset;
  generator::OpentableDataset m_opentableDataset;
  generator::ViatorDataset m_viatorDataset;
  shared_ptr<generator::OsmIdToBoundariesTable> m_boundariesTable;

  /// Used to prepare a list of cities to serve as a list of nodes
  /// for building a highway graph with OSRM for low zooms.
  m4::Tree<Place> m_places;

  enum TypeIndex
  {
    NATURAL_COASTLINE,
    NATURAL_LAND,
    PLACE_ISLAND,
    PLACE_ISLET,

    TYPES_COUNT
  };
  uint32_t m_types[TYPES_COUNT];

  uint32_t Type(TypeIndex i) const { return m_types[i]; }

  uint32_t GetPlaceType(FeatureParams const & params) const
  {
    static uint32_t const placeType = classif().GetTypeByPath({"place"});
    return params.FindType(placeType, 1);
  }

  void UnionEqualPlacesIds(Place const & place)
  {
    if (!m_boundariesTable)
      return;

    auto const id = place.GetFeature().GetLastOsmId();
    m_places.ForEachInRect(place.GetLimitRect(), [&](Place const & p) {
      if (p.IsEqual(place))
        m_boundariesTable->Union(p.GetFeature().GetLastOsmId(), id);
    });
  }

public:
  MainFeaturesEmitter(feature::GenerateInfo const & info)
    : m_skippedElementsPath(info.GetIntermediateFileName("skipped_elements", ".lst"))
    , m_failOnCoasts(info.m_failOnCoasts)
    , m_bookingDataset(info.m_bookingDatafileName, info.m_bookingReferenceDir)
    , m_opentableDataset(info.m_opentableDatafileName, info.m_opentableReferenceDir)
    , m_viatorDataset(info.m_viatorDatafileName)
    , m_boundariesTable(info.m_boundariesTable)
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
      m_countries = my::make_unique<TCountriesGenerator>(info);

    if (info.m_createWorld)
      m_world.reset(new TWorldGenerator(info));
  }

  void operator()(FeatureBuilder1 & fb) override
  {
    uint32_t const type = GetPlaceType(fb.GetParams());

    // TODO(mgserigio): Would it be better to have objects that store callback
    // and can be piped: action-if-cond1 | action-if-cond-2 | ... ?
    // The first object which perform action terminates the cahin.
    if (type != ftype::GetEmptyValue() && !fb.GetName().empty())
    {
      auto const viatorObjId = m_viatorDataset.FindMatchingObjectId(fb);
      if (viatorObjId != generator::ViatorCity::InvalidObjectId())
      {
        m_viatorDataset.PreprocessMatchedOsmObject(viatorObjId, fb, [this, viatorObjId](FeatureBuilder1 & fb)
        {
          m_skippedElements << "VIATOR\t" << DebugPrint(fb.GetMostGenericOsmId())
                            << '\t' << viatorObjId.Get() << endl;
        });
      }

      Place const place(fb, type);
      UnionEqualPlacesIds(place);
      m_places.ReplaceEqualInRect(
          place, [](Place const & p1, Place const & p2) { return p1.IsEqual(p2); },
          [](Place const & p1, Place const & p2) { return p1.IsBetterThan(p2); });
      return;
    }

    auto const bookingObjId = m_bookingDataset.FindMatchingObjectId(fb);
    if (bookingObjId != generator::BookingHotel::InvalidObjectId())
    {
      m_bookingDataset.PreprocessMatchedOsmObject(bookingObjId, fb, [this, bookingObjId](FeatureBuilder1 & fb)
      {
        m_skippedElements << "BOOKING\t" << DebugPrint(fb.GetMostGenericOsmId())
                          << '\t' << bookingObjId.Get() << endl;
        Emit(fb);
      });
      return;
    }

    auto const opentableObjId = m_opentableDataset.FindMatchingObjectId(fb);
    if (opentableObjId != generator::OpentableRestaurant::InvalidObjectId())
    {
      m_opentableDataset.PreprocessMatchedOsmObject(opentableObjId, fb, [this, opentableObjId](FeatureBuilder1 & fb)
      {
        m_skippedElements << "OPENTABLE\t" << DebugPrint(fb.GetMostGenericOsmId())
                          << '\t' << opentableObjId.Get() << endl;
        Emit(fb);
      });
      return;
    }

    Emit(fb);
  }

  void EmitCityBoundary(FeatureBuilder1 const & fb, FeatureParams const & params) override
  {
    if (!m_boundariesTable)
      return;

    auto const type = GetPlaceType(params);
    if (type == ftype::GetEmptyValue())
      return;

    auto const id = fb.GetLastOsmId();
    m_boundariesTable->Append(id, indexer::CityBoundary(fb.GetOuterGeometry()));

    Place const place(fb, type);
    UnionEqualPlacesIds(place);
  }

  /// @return false if coasts are not merged and FLAG_fail_on_coasts is set
  bool Finish() override
  {
    DumpSkippedElements();

    // Emit all required booking objects to the map.
    m_bookingDataset.BuildOsmObjects([this](FeatureBuilder1 & fb) { Emit(fb); });
    // No opentable objects should be emitted. Opentable data enriches some data
    // with a link to a restaurant's reservation page.

    m_places.ForEach([this](Place const & p)
    {
      // m_places are no longer used after this point.
      Emit(const_cast<FeatureBuilder1 &>(p.GetFeature()));
    });

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
      CHECK(m_countries, ());

      feature::ForEachFromDatRawFormat(m_srcCoastsFile, [this](FeatureBuilder1 fb, uint64_t)
      {
        auto & emitter = m_countries->Parent();

        emitter.Start();
        (*m_countries)(fb);
        emitter.Finish();

        if (m_coastsHolder)
        {
          fb.AddName("default", emitter.m_currentNames);
          (*m_coastsHolder)(fb);
        }
      });
    }
    return true;
  }

  void GetNames(vector<string> & names) const override
  {
    if (m_countries)
      names = m_countries->Parent().Names();
    else
      names.clear();
  }

private:
  void Emit(FeatureBuilder1 & fb)
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

  void DumpSkippedElements()
  {
    auto const skippedElements = m_skippedElements.str();

    if (skippedElements.empty())
    {
      LOG(LINFO, ("No osm object was skipped."));
      return;
    }

    ofstream file(m_skippedElementsPath, ios_base::app);
    if (file.is_open())
    {
      file << m_skippedElements.str();
      LOG(LINFO, ("Saving skipped elements to", m_skippedElementsPath, "done."));
    }
    else
    {
      LOG(LERROR, ("Can't output into", m_skippedElementsPath));
    }
  }
};
}  // anonymous namespace

unique_ptr<EmitterBase> MakeMainFeatureEmitter(feature::GenerateInfo const & info)
{
  LOG(LINFO, ("Processing booking data from", info.m_bookingDatafileName, "done."));
  return my::make_unique<MainFeaturesEmitter>(info);
}

template <typename TElement, typename TCache>
void AddElementToCache(TCache & cache, TElement const & em)
{
  switch (em.type)
  {
    case TElement::EntityType::Node:
    {
      auto const pt = MercatorBounds::FromLatLon(em.lat, em.lon);
      cache.AddNode(em.id, pt.y, pt.x);
      break;
    }
    case TElement::EntityType::Way:
    {
      // store way
      WayElement way(em.id);
      for (uint64_t nd : em.Nodes())
        way.nodes.push_back(nd);

      if (way.IsValid())
        cache.AddWay(em.id, way);
      break;
    }
    case TElement::EntityType::Relation:
    {
      // store relation
      RelationElement relation;
      for (auto const & member : em.Members())
      {
        if (member.type == TElement::EntityType::Node)
          relation.nodes.emplace_back(make_pair(member.ref, string(member.role)));
        else if (member.type == TElement::EntityType::Way)
          relation.ways.emplace_back(make_pair(member.ref, string(member.role)));
        // we just ignore type == "relation"
      }

      for (auto const & tag : em.Tags())
        relation.tags.emplace(make_pair(string(tag.key), string(tag.value)));

      if (relation.IsValid())
        cache.AddRelation(em.id, relation);

      break;
    }
    default:
      break;
  }
}

template <typename TCache>
void BuildIntermediateDataFromXML(SourceReader & stream, TCache & cache, TownsDumper & towns)
{
  XMLSource parser([&](OsmElement * e)
    {
      towns.CheckElement(*e);
      AddElementToCache(cache, *e);
    });
  ParseXMLSequence(stream, parser);
}

void ProcessOsmElementsFromXML(SourceReader & stream, function<void(OsmElement *)> processor)
{
  XMLSource parser([&](OsmElement * e) { processor(e); });
  ParseXMLSequence(stream, parser);
}

template <typename TCache>
void BuildIntermediateDataFromO5M(SourceReader & stream, TCache & cache, TownsDumper & towns)
{
  osm::O5MSource dataset([&stream](uint8_t * buffer, size_t size)
  {
    return stream.Read(reinterpret_cast<char *>(buffer), size);
  });

  for (auto const & e : dataset)
  {
    towns.CheckElement(e);
    AddElementToCache(cache, e);
  }
}

void ProcessOsmElementsFromO5M(SourceReader & stream, function<void(OsmElement *)> processor)
{
  using TType = osm::O5MSource::EntityType;

  osm::O5MSource dataset([&stream](uint8_t * buffer, size_t size)
  {
    return stream.Read(reinterpret_cast<char *>(buffer), size);
  });

  auto translate = [](TType t) -> OsmElement::EntityType
  {
    switch (t)
    {
      case TType::Node: return OsmElement::EntityType::Node;
      case TType::Way: return OsmElement::EntityType::Way;
      case TType::Relation: return OsmElement::EntityType::Relation;
      default: return OsmElement::EntityType::Unknown;
    }
  };

  for (auto const & em : dataset)
  {
    OsmElement p;
    p.id = em.id;

    switch (em.type)
    {
      case TType::Node:
      {
        p.type = OsmElement::EntityType::Node;
        p.lat = em.lat;
        p.lon = em.lon;
        break;
      }
      case TType::Way:
      {
        p.type = OsmElement::EntityType::Way;
        for (uint64_t nd : em.Nodes())
          p.AddNd(nd);
        break;
      }
      case TType::Relation:
      {
        p.type = OsmElement::EntityType::Relation;
        for (auto const & member : em.Members())
          p.AddMember(member.ref, translate(member.type), member.role);
        break;
      }
      default: break;
    }

    for (auto const & tag : em.Tags())
      p.AddTag(tag.key, tag.value);

    processor(&p);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Generate functions implementations.
///////////////////////////////////////////////////////////////////////////////////////////////////

template <class TNodesHolder>
bool GenerateFeaturesImpl(feature::GenerateInfo & info, EmitterBase & emitter)
{
  try
  {
    TNodesHolder nodes(info.GetIntermediateFileName(NODES_FILE, ""));

    using TDataCache = IntermediateData<TNodesHolder, cache::EMode::Read>;
    TDataCache cache(nodes, info);
    cache.LoadIndex();

    // TODO(mgsergio): Get rid of EmitterBase template parameter.
    OsmToFeatureTranslator<EmitterBase, TDataCache> parser(
        emitter, cache, info.m_makeCoasts ? classif().GetCoastType() : 0,
        info.GetAddressesFileName(), info.GetIntermediateFileName(RESTRICTIONS_FILENAME, ""),
        info.GetIntermediateFileName(ROAD_ACCESS_FILENAME, ""),
        info.GetIntermediateFileName(METALINES_FILENAME, ""));

    TagAdmixer tagAdmixer(info.GetIntermediateFileName("ways", ".csv"),
                          info.GetIntermediateFileName("towns", ".csv"));
    TagReplacer tagReplacer(GetPlatform().ResourcesDir() + REPLACED_TAGS_FILE);
    OsmTagMixer osmTagMixer(GetPlatform().ResourcesDir() + MIXED_TAGS_FILE);

    // Here we can add new tags to the elements!
    auto const fn = [&](OsmElement * e)
    {
      tagReplacer(e);
      tagAdmixer(e);
      osmTagMixer(e);

      parser.EmitElement(e);
    };

    SourceReader reader = info.m_osmFileName.empty() ? SourceReader() : SourceReader(info.m_osmFileName);
    switch (info.m_osmFileType)
    {
      case feature::GenerateInfo::OsmSourceType::XML:
        ProcessOsmElementsFromXML(reader, fn);
        break;
      case feature::GenerateInfo::OsmSourceType::O5M:
        ProcessOsmElementsFromO5M(reader, fn);
        break;
    }

    LOG(LINFO, ("Processing", info.m_osmFileName, "done."));

    // Stop if coasts are not merged and FLAG_fail_on_coasts is set
    if (!emitter.Finish())
      return false;

    emitter.GetNames(info.m_bucketNames);
  }
  catch (Reader::Exception const & ex)
  {
    LOG(LCRITICAL, ("Error with file", ex.Msg()));
  }

  return true;
}

template <class TNodesHolder>
bool GenerateIntermediateDataImpl(feature::GenerateInfo & info)
{
  try
  {
    TNodesHolder nodes(info.GetIntermediateFileName(NODES_FILE, ""));
    using TDataCache = IntermediateData<TNodesHolder, cache::EMode::Write>;
    TDataCache cache(nodes, info);
    TownsDumper towns;

    SourceReader reader = info.m_osmFileName.empty() ? SourceReader() : SourceReader(info.m_osmFileName);

    LOG(LINFO, ("Data source:", info.m_osmFileName));

    switch (info.m_osmFileType)
    {
      case feature::GenerateInfo::OsmSourceType::XML:
        BuildIntermediateDataFromXML(reader, cache, towns);
        break;
      case feature::GenerateInfo::OsmSourceType::O5M:
        BuildIntermediateDataFromO5M(reader, cache, towns);
        break;
    }

    cache.SaveIndex();
    towns.Dump(info.GetIntermediateFileName(TOWNS_FILE, ""));
    LOG(LINFO, ("Added points count = ", nodes.GetProcessedPoint()));
  }
  catch (Writer::Exception const & e)
  {
    LOG(LCRITICAL, ("Error with file ", e.what()));
  }
  return true;
}

bool GenerateFeatures(feature::GenerateInfo & info, EmitterFactory factory)
{
  auto emitter = factory(info);
  switch (info.m_nodeStorageType)
  {
    case feature::GenerateInfo::NodeStorageType::File:
      return GenerateFeaturesImpl<cache::RawFilePointStorage<cache::EMode::Read>>(info, *emitter);
    case feature::GenerateInfo::NodeStorageType::Index:
      return GenerateFeaturesImpl<cache::MapFilePointStorage<cache::EMode::Read>>(info, *emitter);
    case feature::GenerateInfo::NodeStorageType::Memory:
      return GenerateFeaturesImpl<cache::RawMemPointStorage<cache::EMode::Read>>(info, *emitter);
  }
  return false;
}

bool GenerateIntermediateData(feature::GenerateInfo & info)
{
  switch (info.m_nodeStorageType)
  {
    case feature::GenerateInfo::NodeStorageType::File:
      return GenerateIntermediateDataImpl<cache::RawFilePointStorage<cache::EMode::Write>>(info);
    case feature::GenerateInfo::NodeStorageType::Index:
      return GenerateIntermediateDataImpl<cache::MapFilePointStorage<cache::EMode::Write>>(info);
    case feature::GenerateInfo::NodeStorageType::Memory:
      return GenerateIntermediateDataImpl<cache::RawMemPointStorage<cache::EMode::Write>>(info);
  }
  return false;
}
