#include "feature_generator.hpp"
#include "data_cache_file.hpp"
#include "osm_element.hpp"
#include "polygonizer.hpp"
#include "osm_decl.hpp"
#include "generate_info.hpp"
#include "coastlines_generator.hpp"
#include "world_map_generator.hpp"

#include "../defines.hpp"

#include "../indexer/data_header.hpp"
#include "../indexer/mercator.hpp"
#include "../indexer/cell_id.hpp"
#include "../indexer/classificator.hpp"

#include "../coding/varint.hpp"

#include "../base/assert.hpp"
#include "../base/logging.hpp"
#include "../base/stl_add.hpp"

#include "../std/bind.hpp"
#include "../std/unordered_map.hpp"
#include "../std/target_os.hpp"

#include "point_storage.hpp"

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

///////////////////////////////////////////////////////////////////////////////////////////////////
// FeaturesCollector implementation
///////////////////////////////////////////////////////////////////////////////////////////////////

FeaturesCollector::FeaturesCollector(string const & fName)
: m_datFile(fName)
{
  CHECK_EQUAL(GetFileSize(m_datFile), 0, ());
}

FeaturesCollector::~FeaturesCollector()
{
  FlushBuffer();
  /// Check file size
  (void)GetFileSize(m_datFile);
}

uint32_t FeaturesCollector::GetFileSize(FileWriter const & f)
{
  // .dat file should be less than 4Gb
  uint64_t const pos = f.Pos();
  uint32_t const ret = static_cast<uint32_t>(pos);

  CHECK_EQUAL(static_cast<uint64_t>(ret), pos, ("Feature offset is out of 32bit boundary!"));
  return ret;
}

template <typename ValueT, size_t ValueSizeT = sizeof(ValueT)+1>
pair<char[ValueSizeT], uint8_t> PackValue(ValueT v)
{
  static_assert(is_integral<ValueT>::value, "Non integral value");
  static_assert(is_unsigned<ValueT>::value, "Non unsigned value");

  pair<char[ValueSizeT], uint8_t> res;
  res.second = 0;
  while (v > 127)
  {
    res.first[res.second++] = static_cast<uint8_t>((v & 127) | 128);
    v >>= 7;
  }
  res.first[res.second++] = static_cast<uint8_t>(v);
  return res;
}

void FeaturesCollector::FlushBuffer()
{
  m_datFile.Write(m_writeBuffer, m_writePosition);
  m_baseOffset += m_writePosition;
  m_writePosition = 0;
}

void FeaturesCollector::Flush()
{
  FlushBuffer();
  m_datFile.Flush();
}

void FeaturesCollector::Write(char const *src, size_t size)
{
  do
  {
    if (m_writePosition == sizeof(m_writeBuffer))
      FlushBuffer();
    size_t const part_size = min(size, sizeof(m_writeBuffer) - m_writePosition);
    memcpy(&m_writeBuffer[m_writePosition], src, part_size);
    m_writePosition += part_size;
    size -= part_size;
    src += part_size;
  } while(size > 0);
}


uint32_t FeaturesCollector::WriteFeatureBase(vector<char> const & bytes, FeatureBuilder1 const & fb)
{
  size_t const sz = bytes.size();
  CHECK(sz != 0, ("Empty feature not allowed here!"));

  size_t const offset = m_baseOffset + m_writePosition;

  auto const & packedSize = PackValue(sz);
  Write(packedSize.first, packedSize.second);
  Write(&bytes[0], sz);

  m_bounds.Add(fb.GetLimitRect());
  CHECK_EQUAL(offset, static_cast<uint32_t>(offset), ());
  return static_cast<uint32_t>(offset);
}

void FeaturesCollector::operator() (FeatureBuilder1 const & fb)
{
  FeatureBuilder1::buffer_t bytes;
  fb.Serialize(bytes);
  (void)WriteFeatureBase(bytes, fb);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Generate functions implementations.
///////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{

class MainFeaturesEmitter
{
  typedef WorldMapGenerator<FeaturesCollector> WorldGenerator;
  typedef CountryMapGenerator<Polygonizer<FeaturesCollector> > CountriesGenerator;
  unique_ptr<CountriesGenerator> m_countries;
  unique_ptr<WorldGenerator> m_world;

  unique_ptr<CoastlineFeaturesGenerator> m_coasts;
  unique_ptr<FeaturesCollector> m_coastsHolder;

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
  MainFeaturesEmitter(GenerateInfo const & info)
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
        m_coastsHolder.reset(new FeaturesCollector(
                info.m_datFilePrefix + WORLD_COASTS_FILE_NAME + info.m_datFileSuffix));
      }
    }
    else
    {
      // 4-10 - level range for cells
      // 20000 - max points count per feature
      m_coasts.reset(new CoastlineFeaturesGenerator(Type(NATURAL_COASTLINE), 4, 10, 20000));

      m_coastsHolder.reset(new FeaturesCollector(m_srcCoastsFile));
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
      CombinedEmitter<FeaturesCollector, CountriesGenerator>
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

}

template <class NodesHolderT>
bool GenerateImpl(GenerateInfo & info, string const & osmFileName = string())
{
  try
  {
    NodesHolderT nodes(info.m_tmpDir + NODES_FILE);

    typedef FileHolder<NodesHolderT> HolderT;
    HolderT holder(nodes, info.m_tmpDir);
    holder.LoadIndex();

    MainFeaturesEmitter bucketer(info);
    SecondPassParser<MainFeaturesEmitter, HolderT> parser(
          bucketer, holder,
          info.m_makeCoasts ? classif().GetCoastType() : 0,
          info.m_addressFile);

    if (osmFileName.empty())
      ParseXMLFromStdIn(parser);
    else
      ParseXMLFromFile(parser, osmFileName);

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

bool GenerateFeatures(GenerateInfo & info, string const & nodeStorage, string const & osmFileName)
{
  if (nodeStorage == "raw")
    return GenerateImpl<RawFileShortPointStorage<BasePointStorage::MODE_READ>>(info, osmFileName);
  else if (nodeStorage == "map")
    return GenerateImpl<MapFileShortPointStorage<BasePointStorage::MODE_READ>>(info, osmFileName);
  else if (nodeStorage == "sqlite")
    return GenerateImpl<SQLitePointStorage<BasePointStorage::MODE_READ>>(info, osmFileName);
  else if (nodeStorage == "mem")
    return GenerateImpl<RawMemShortPointStorage<BasePointStorage::MODE_READ>>(info, osmFileName);
  else
    CHECK(nodeStorage.empty(), ("Incorrect node_storage type:", nodeStorage));
  return false;
}

}
