#include "generator/intermediate_data.hpp"

#include "base/checked_cast.hpp"

#include <future>
#include <execution>

namespace generator::cache
{
using std::string;

namespace
{
size_t const kFlushCount = 1024;
double const kValueOrder = 1e7;
string const kShortExtension = ".short";

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

protected:
  uint64_t m_numProcessedPoints = 0;
};

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

class RawFilePointStorageWriter : public PointStorageWriterBase
{
public:
  explicit RawFilePointStorageWriter(string const & name)
    : m_fileWriter(name)
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
};

class RawMemPointStorageReader : public PointStorageReaderInterface
{
public:
  explicit RawMemPointStorageReader(string const & name)
    : m_fileReader(name)
  {
    uint64_t const fileSize = m_fileReader.Size();
    CHECK_EQUAL(fileSize % sizeof(LatLon), 0, ("Node's coordinates file is broken"));

    m_data.resize(fileSize / sizeof(LatLon));
    m_fileReader.Read(0, m_data.data(), fileSize);
  }

  // PointStorageReaderInterface overrides:
  bool GetPoint(uint64_t id, double & lat, double & lon) const override
  {
    LatLon const & ll = m_data[id];
    bool const ret = FromLatLon(ll, lat, lon);
    if (!ret)
      LOG(LERROR, ("Node with id =", id, "not found!"));
    return ret;
  }

private:
  FileReader m_fileReader;
  std::vector<LatLon> m_data;
};

class RawMemPointStorageWriter : public PointStorageWriterBase
{
  // 16G buffer size.
  static constexpr size_t kBufferSize = 1000000000;

public:
  explicit RawMemPointStorageWriter(string const & name)
    : m_fileWriter(name)
  {
    m_buffer.reserve(kBufferSize);
  }

  ~RawMemPointStorageWriter() noexcept(false) override
  {
    Flush();
  }

  // PointStorageWriterInterface overrides:
  void AddPoint(uint64_t id, double lat, double lon) override
  {
    if (m_buffer.size() >= kBufferSize)
      FlushAsync();

    LatLon ll;
    ToLatLon(lat, lon, ll);
    m_buffer.push_back({id, ll});

    ++m_numProcessedPoints;
  }

private:
  using BufferT = std::vector<LatLonPos>;

  void FlushImpl(BufferT & buffer)
  {
    // Sort, according to the seek pos in file.

    /// @todo Try parallel sort when clang will be able.
    //std::sort(std::execution::par, buffer.begin(), buffer.end(), [](LatLonPos const & l, LatLonPos const & r)
    std::sort(buffer.begin(), buffer.end(), [](LatLonPos const & l, LatLonPos const & r)
    {
      return l.m_pos < r.m_pos;
    });

    size_t constexpr structSize = sizeof(LatLon);
    for (auto const & llp : buffer)
      m_fileWriter.Write(llp.m_pos * structSize, &llp.m_ll, structSize);
  }

  // Async version to continue collecting points in parallel, while writing a file.
  void FlushAsync()
  {
    if (m_future.valid())
      m_future.wait();

    BufferT * pBuffer = new BufferT(std::move(m_buffer));
    m_buffer.clear();
    m_buffer.reserve(kBufferSize);

    // Using raw pointers because we can't make std::function with rvalue reference.
    m_future = std::async(std::launch::async, [this, pBuffer]()
    {
      FlushImpl(*pBuffer);
      delete pBuffer;
    });
  }

  void Flush()
  {
    if (m_future.valid())
      m_future.wait();

    FlushImpl(m_buffer);
    m_buffer.clear();
  }

private:
  // Expect that fseek(FILE) makes the same check inside, but no ...
  class CachedPosWriter
  {
    FileWriter m_writer;
    uint64_t m_pos = 0;

  public:
    explicit CachedPosWriter(std::string const & fPath) : m_writer(fPath)
    {
      CHECK_EQUAL(m_pos, m_writer.Pos(), ());
    }

    void Write(uint64_t pos, void const * p, size_t size)
    {
      if (m_pos != pos)
      {
        m_writer.Seek(pos);
        m_pos = pos;
      }

      m_writer.Write(p, size);
      m_pos += size;
    }
  };

  CachedPosWriter m_fileWriter;
  BufferT m_buffer;
  std::future<void> m_future;
};

class MapFilePointStorageReader : public PointStorageReaderInterface
{
public:
  explicit MapFilePointStorageReader(string const & name)
    : m_fileReader(name + kShortExtension)
  {
    LOG(LINFO, ("Nodes reading is started"));

    uint64_t const count = m_fileReader.Size();

    uint64_t pos = 0;
    LatLonPos llp;
    while (pos < count)
    {
      m_fileReader.Read(pos, &llp, sizeof(llp));
      pos += sizeof(llp);

      m_map.emplace(llp.m_pos, llp.m_ll);
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

class MapFilePointStorageWriter : public PointStorageWriterBase
{
public:
  explicit MapFilePointStorageWriter(string const & name)
    : m_fileWriter(name + kShortExtension)
  {
  }

  // PointStorageWriterInterface overrides:
  void AddPoint(uint64_t id, double lat, double lon) override
  {
    LatLonPos llp;
    llp.m_pos = id;

    ToLatLon(lat, lon, llp.m_ll);

    m_fileWriter.Write(&llp, sizeof(llp));

    ++m_numProcessedPoints;
  }

private:
  FileWriter m_fileWriter;
};
}  // namespace

// IndexFileReader ---------------------------------------------------------------------------------
IndexFileReader::IndexFileReader(string const & name)
{
  FileReader fileReader(name);
  m_elements.clear();
  size_t const fileSize = base::checked_cast<size_t>(fileReader.Size());
  if (fileSize == 0)
    return;

  LOG_SHORT(LINFO, ("Offsets reading is started for file", fileReader.GetName()));
  CHECK_EQUAL(0, fileSize % sizeof(Element), ("Damaged file."));

  try
  {
    m_elements.resize(fileSize / sizeof(Element));
  }
  catch (std::bad_alloc const &)
  {
    LOG(LCRITICAL, ("Insufficient memory for required offset map"));
  }

  fileReader.Read(0, &m_elements[0], fileSize);

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

// IntermediateDataObjectsCache --------------------------------------------------------------------
IntermediateDataObjectsCache::AllocatedObjects &
IntermediateDataObjectsCache::GetOrCreatePointStorageReader(
    feature::GenerateInfo::NodeStorageType type, string const & name)
{
  auto const strType = std::to_string(static_cast<int>(type));
  auto const key = strType + name;

  std::lock_guard lock(m_mutex);

  auto res = m_objects.try_emplace(key, type, name);
  if (res.second)
    LOG(LINFO, ("Created nodes reader:", strType, name));
  return res.first->second;
}

void IntermediateDataObjectsCache::Clear()
{
  std::lock_guard lock(m_mutex);
  std::unordered_map<string, AllocatedObjects>().swap(m_objects);
}

IntermediateDataObjectsCache::AllocatedObjects::AllocatedObjects(
    feature::GenerateInfo::NodeStorageType type, string const & name)
{
  m_storageReader = CreatePointStorageReader(type, name);
}

IndexFileReader const & IntermediateDataObjectsCache::AllocatedObjects::GetOrCreateIndexReader(
    std::string const & name)
{
  static std::mutex m;
  std::lock_guard lock(m);

  return m_fileReaders.try_emplace(name, name).first->second;
}

// IntermediateDataReader --------------------------------------------------------------------------
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

// IntermediateDataWriter --------------------------------------------------------------------------
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
