#include "generator/intermediate_data.hpp"

#include <new>
#include <set>
#include <string>

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

namespace generator::cache
{
using std::string;

namespace
{
size_t const kFlushCount = 1024;
double const kValueOrder = 1e7;
string const kShortExtension = ".short";

// An estimation.
// OSM had around 4.1 billion nodes on 2017-11-08,
// see https://wiki.openstreetmap.org/wiki/Stats
size_t const kMaxNodesInOSM = size_t{1} << 33;

void ToLatLon(double lat, double lon, LatLon & ll)
{
  int64_t const lat64 = lat * kValueOrder;
  int64_t const lon64 = lon * kValueOrder;

  CHECK(lat64 >= std::numeric_limits<int32_t>::min() && lat64 <= std::numeric_limits<int32_t>::max(),
        ("Latitude is out of 32bit boundary:", lat64));
  CHECK(lon64 >= std::numeric_limits<int32_t>::min() && lon64 <= std::numeric_limits<int32_t>::max(),
        ("Longitude is out of 32bit boundary:", lon64));
  ll.m_lat = static_cast<int32_t>(lat64);
  ll.m_lon = static_cast<int32_t>(lon64);
}

bool FromLatLon(LatLon const & ll, double & lat, double & lon)
{
  // Assume that a valid coordinate is not (0, 0).
  if (ll.m_lat != 0.0 || ll.m_lon != 0.0)
  {
    lat = static_cast<double>(ll.m_lat) / kValueOrder;
    lon = static_cast<double>(ll.m_lon) / kValueOrder;
    return true;
  }
  lat = 0.0;
  lon = 0.0;
  return false;
}

template <class Index, class Container>
void AddToIndex(Index & index, Key relationId, Container const & values)
{
  for (auto const & v : values)
    index.Add(v.first, relationId);
}

class PointStorageWriterBase : public PointStorageWriterInterface
{
public:
  // PointStorageWriterInterface overrides:
  uint64_t GetNumProcessedPoints() const override { return m_numProcessedPoints; }

private:
  uint64_t m_numProcessedPoints;
};

// RawFilePointStorageMmapReader -------------------------------------------------------------------
class RawFilePointStorageMmapReader : public PointStorageReaderInterface
{
public:
  explicit RawFilePointStorageMmapReader(string const & name)
    : m_mmapReader(name, MmapReader::Advice::Random)
  {}

  // PointStorageReaderInterface overrides:
  bool GetPoint(uint64_t id, double & lat, double & lon) const override
  {
    LatLon ll;
    m_mmapReader.Read(id * sizeof(ll), &ll, sizeof(ll));

    bool ret = FromLatLon(ll, lat, lon);
    if (!ret)
      LOG(LERROR, ("Node with id =", id, "not found!"));
    return ret;
  }

private:
  MmapReader m_mmapReader;
};

// RawFilePointStorageWriter -----------------------------------------------------------------------
class RawFilePointStorageWriter : public PointStorageWriterBase
{
public:
  explicit RawFilePointStorageWriter(string const & name) :
    m_fileWriter(name)
  {}

  // PointStorageWriterInterface overrides:
  void AddPoint(uint64_t id, double lat, double lon) override
  {
    LatLon ll;
    ToLatLon(lat, lon, ll);

    m_fileWriter.Seek(id * sizeof(ll));
    m_fileWriter.Write(&ll, sizeof(ll));

    ++m_numProcessedPoints;
  }

private:
  FileWriter m_fileWriter;
  uint64_t m_numProcessedPoints = 0;
};

// RawMemPointStorageReader ------------------------------------------------------------------------
class RawMemPointStorageReader : public PointStorageReaderInterface
{
public:
  explicit RawMemPointStorageReader(string const & name):
    m_fileReader(name),
    m_data(kMaxNodesInOSM)
  {
    static_assert(sizeof(size_t) == 8, "This code is only for 64-bit architectures");
    m_fileReader.Read(0, m_data.data(), m_data.size() * sizeof(LatLon));
  }

  // PointStorageReaderInterface overrides:
  bool GetPoint(uint64_t id, double & lat, double & lon) const override
  {
    LatLon const & ll = m_data[id];
    bool ret = FromLatLon(ll, lat, lon);
    if (!ret)
      LOG(LERROR, ("Node with id =", id, "not found!"));
    return ret;
  }

private:
  FileReader m_fileReader;
  std::vector<LatLon> m_data;
};

// RawMemPointStorageWriter ------------------------------------------------------------------------
class RawMemPointStorageWriter : public PointStorageWriterBase
{
public:
  explicit RawMemPointStorageWriter(string const & name) :
    m_fileWriter(name),
    m_data(kMaxNodesInOSM)
  {
  }

  ~RawMemPointStorageWriter() noexcept(false) override
  {
    m_fileWriter.Write(m_data.data(), m_data.size() * sizeof(LatLon));
  }

  // PointStorageWriterInterface overrides:
  void AddPoint(uint64_t id, double lat, double lon) override
  {
    CHECK_LESS(id, m_data.size(),
               ("Found node with id", id, "which is bigger than the allocated cache size"));

    LatLon & ll = m_data[id];
    ToLatLon(lat, lon, ll);

    ++m_numProcessedPoints;
  }

private:
  FileWriter m_fileWriter;
  std::vector<LatLon> m_data;
  uint64_t m_numProcessedPoints = 0;
};

// MapFilePointStorageReader -----------------------------------------------------------------------
class MapFilePointStorageReader : public PointStorageReaderInterface
{
public:
  explicit MapFilePointStorageReader(string const & name) :
    m_fileReader(name + kShortExtension)
  {
    LOG(LINFO, ("Nodes reading is started"));

    uint64_t const count = m_fileReader.Size();

    uint64_t pos = 0;
    LatLonPos llp;
    LatLon ll;
    while (pos < count)
    {
      m_fileReader.Read(pos, &llp, sizeof(llp));
      pos += sizeof(llp);

      ll.m_lat = llp.m_lat;
      ll.m_lon = llp.m_lon;
      m_map.emplace(llp.m_pos, ll);
    }

    LOG(LINFO, ("Nodes reading is finished"));
  }

  // PointStorageReaderInterface overrides:
  bool GetPoint(uint64_t id, double & lat, double & lon) const override
  {
    auto const i = m_map.find(id);
    if (i == m_map.cend())
      return false;
    bool ret = FromLatLon(i->second, lat, lon);
    if (!ret)
    {
      LOG(LERROR, ("Inconsistent MapFilePointStorageReader. Node with id =", id,
                   "must exist but was not found"));
    }
    return ret;
  }

private:
  FileReader m_fileReader;
  std::unordered_map<uint64_t, LatLon> m_map;
};

// MapFilePointStorageWriter -----------------------------------------------------------------------
class MapFilePointStorageWriter : public PointStorageWriterBase
{
public:
  explicit MapFilePointStorageWriter(string const & name) :
    m_fileWriter(name + kShortExtension)
  {
  }

  // PointStorageWriterInterface overrides:
  void AddPoint(uint64_t id, double lat, double lon) override
  {
    LatLon ll;
    ToLatLon(lat, lon, ll);

    LatLonPos llp;
    llp.m_pos = id;
    llp.m_lat = ll.m_lat;
    llp.m_lon = ll.m_lon;

    m_fileWriter.Write(&llp, sizeof(llp));

    ++m_numProcessedPoints;
  }

private:
  FileWriter m_fileWriter;
  uint64_t m_numProcessedPoints = 0;
};
}  // namespace

// IndexFileReader ---------------------------------------------------------------------------------
IndexFileReader::IndexFileReader(string const & name)
{
  FileReader fileReader(name);
  m_elements.clear();
  size_t const fileSize = fileReader.Size();
  if (fileSize == 0)
    return;

  LOG_SHORT(LINFO, ("Offsets reading is started for file", fileReader.GetName()));
  CHECK_EQUAL(0, fileSize % sizeof(Element), ("Damaged file."));

  try
  {
    m_elements.resize(base::checked_cast<size_t>(fileSize / sizeof(Element)));
  }
  catch (std::bad_alloc const &)
  {
    LOG(LCRITICAL, ("Insufficient memory for required offset map"));
  }

  fileReader.Read(0, &m_elements[0], base::checked_cast<size_t>(fileSize));

  sort(m_elements.begin(), m_elements.end(), ElementComparator());

  LOG_SHORT(LINFO, ("Offsets reading is finished"));
}

bool IndexFileReader::GetValueByKey(Key key, Value & value) const
{
  auto it = lower_bound(m_elements.begin(), m_elements.end(), key, ElementComparator());
  if (it != m_elements.end() && it->first == key)
  {
    value = it->second;
    return true;
  }
  return false;
}

// IndexFileWriter ---------------------------------------------------------------------------------
IndexFileWriter::IndexFileWriter(string const & name) :
  m_fileWriter(name)
{
}

void IndexFileWriter::WriteAll()
{
  if (m_elements.empty())
    return;

  m_fileWriter.Write(&m_elements[0], m_elements.size() * sizeof(Element));
  m_elements.clear();
}

void IndexFileWriter::Add(Key k, Value const & v)
{
  if (m_elements.size() > kFlushCount)
    WriteAll();

  m_elements.emplace_back(k, v);
}

// OSMElementCacheReader ---------------------------------------------------------------------------
OSMElementCacheReader::OSMElementCacheReader(IntermediateDataObjectsCache::AllocatedObjects & allocatedObjects,
                                             string const & name, bool preload)
  : m_fileReader(name)
  , m_offsetsReader(allocatedObjects.GetOrCreateIndexReader(name + OFFSET_EXT))
  , m_name(name)
  , m_preload(preload)
{
  if (!m_preload)
    return;
  size_t sz = m_fileReader.Size();
  m_data.resize(sz);
  m_fileReader.Read(0, m_data.data(), sz);
}

// OSMElementCacheWriter ---------------------------------------------------------------------------
OSMElementCacheWriter::OSMElementCacheWriter(string const & name)
  : m_fileWriter(name), m_offsets(name + OFFSET_EXT), m_name(name)
{
}

void OSMElementCacheWriter::SaveOffsets() { m_offsets.WriteAll(); }

IntermediateDataObjectsCache::AllocatedObjects &
IntermediateDataObjectsCache::GetOrCreatePointStorageReader(
    feature::GenerateInfo::NodeStorageType type, string const & name)
{
  static std::mutex m;
  auto const k = std::to_string(static_cast<int>(type)) + name;
  std::lock_guard lock(m);
  auto it = m_objects.find(k);
  if (it == cend(m_objects))
    return m_objects.emplace(k, AllocatedObjects(CreatePointStorageReader(type, name))).first->second;

  return it->second;
}

void IntermediateDataObjectsCache::Clear()
{
  m_objects = std::unordered_map<std::string, AllocatedObjects>();
}

IndexFileReader const & IntermediateDataObjectsCache::AllocatedObjects::GetOrCreateIndexReader(
    std::string const & name)
{
  static std::mutex m;
  std::lock_guard lock(m);
  auto it = m_fileReaders.find(name);
  if (it == cend(m_fileReaders))
    return m_fileReaders.emplace(name, IndexFileReader(name)).first->second;

  return it->second;
}

// IntermediateDataReader
IntermediateDataReader::IntermediateDataReader(
    IntermediateDataObjectsCache::AllocatedObjects & objs, feature::GenerateInfo const & info)
  : m_nodes(objs.GetPointStorageReader())
  , m_ways(objs, info.GetCacheFileName(WAYS_FILE), info.m_preloadCache)
  , m_relations(objs, info.GetCacheFileName(RELATIONS_FILE), info.m_preloadCache)
  , m_nodeToRelations(objs.GetOrCreateIndexReader(info.GetCacheFileName(NODES_FILE, ID2REL_EXT)))
  , m_wayToRelations(objs.GetOrCreateIndexReader(info.GetCacheFileName(WAYS_FILE, ID2REL_EXT)))
  , m_relationToRelations(
        objs.GetOrCreateIndexReader(info.GetCacheFileName(RELATIONS_FILE, ID2REL_EXT)))
{}

// IntermediateDataWriter
IntermediateDataWriter::IntermediateDataWriter(PointStorageWriterInterface & nodes,
                                               feature::GenerateInfo const & info)
  : m_nodes(nodes)
  , m_ways(info.GetCacheFileName(WAYS_FILE))
  , m_relations(info.GetCacheFileName(RELATIONS_FILE))
  , m_nodeToRelations(info.GetCacheFileName(NODES_FILE, ID2REL_EXT))
  , m_wayToRelations(info.GetCacheFileName(WAYS_FILE, ID2REL_EXT))
  , m_relationToRelations(info.GetCacheFileName(RELATIONS_FILE, ID2REL_EXT))
{}

void IntermediateDataWriter::AddRelation(Key id, RelationElement const & e)
{
  static std::set<std::string_view> const types = {"multipolygon", "route", "boundary",
                                    "associatedStreet", "building", "restriction"};
  auto const relationType = e.GetType();
  if (!types.count(relationType))
    return;

  m_relations.Write(id, e);
  AddToIndex(m_nodeToRelations, id, e.m_nodes);
  AddToIndex(m_wayToRelations, id, e.m_ways);
  AddToIndex(m_relationToRelations, id, e.m_relations);
}

void IntermediateDataWriter::SaveIndex()
{
  m_ways.SaveOffsets();
  m_relations.SaveOffsets();

  m_nodeToRelations.WriteAll();
  m_wayToRelations.WriteAll();
  m_relationToRelations.WriteAll();
}

// Functions
std::unique_ptr<PointStorageReaderInterface>
CreatePointStorageReader(feature::GenerateInfo::NodeStorageType type, string const & name)
{
  switch (type)
  {
  case feature::GenerateInfo::NodeStorageType::File:
    return std::make_unique<RawFilePointStorageMmapReader>(name);
  case feature::GenerateInfo::NodeStorageType::Index:
    return std::make_unique<MapFilePointStorageReader>(name);
  case feature::GenerateInfo::NodeStorageType::Memory:
    return std::make_unique<RawMemPointStorageReader>(name);
  }
  UNREACHABLE();
}

std::unique_ptr<PointStorageWriterInterface>
CreatePointStorageWriter(feature::GenerateInfo::NodeStorageType type, string const & name)
{
  switch (type)
  {
  case feature::GenerateInfo::NodeStorageType::File:
    return std::make_unique<RawFilePointStorageWriter>(name);
  case feature::GenerateInfo::NodeStorageType::Index:
    return std::make_unique<MapFilePointStorageWriter>(name);
  case feature::GenerateInfo::NodeStorageType::Memory:
    return std::make_unique<RawMemPointStorageWriter>(name);
  }
  UNREACHABLE();
}

IntermediateData::IntermediateData(IntermediateDataObjectsCache & objectsCache,
                                   feature::GenerateInfo const & info)
  : m_objectsCache(objectsCache)
  , m_info(info)
{
  auto & allocatedObjects = m_objectsCache.GetOrCreatePointStorageReader(
      info.m_nodeStorageType, info.GetCacheFileName(NODES_FILE));
  m_reader = std::make_shared<IntermediateDataReader>(allocatedObjects, info);
}

std::shared_ptr<IntermediateDataReader> const & IntermediateData::GetCache() const
{
  return m_reader;
}

std::shared_ptr<IntermediateData> IntermediateData::Clone() const
{
  return std::make_shared<IntermediateData>(m_objectsCache, m_info);
}
}  // namespace generator
