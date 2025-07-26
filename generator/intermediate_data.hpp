#pragma once

#include "generator/generate_info.hpp"
#include "generator/intermediate_elements.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/mmap_reader.hpp"

#include "base/assert.hpp"
#include "base/control_flow.hpp"
#include "base/file_name_utils.hpp"
#include "base/logging.hpp"

#include "defines.hpp"

#include <algorithm>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

// Classes for reading and writing any data in file with map of offsets for
// fast searching in memory by some key.
namespace generator
{
namespace cache
{
using Key = uint64_t;
static_assert(std::is_integral<Key>::value, "Key must be an integral type");

// Used to store all world nodes inside temporary index file.
// To find node by id, just calculate offset inside index file:
// offset_in_file = sizeof(LatLon) * node_ID
struct LatLon
{
  int32_t m_lat = 0;
  int32_t m_lon = 0;
};
static_assert(sizeof(LatLon) == 8, "Invalid structure size");
static_assert(std::is_trivially_copyable<LatLon>::value, "");

struct LatLonPos
{
  uint64_t m_pos = 0;
  LatLon m_ll;
};
static_assert(sizeof(LatLonPos) == 16, "Invalid structure size");
static_assert(std::is_trivially_copyable<LatLonPos>::value, "");

class PointStorageWriterInterface
{
public:
  virtual ~PointStorageWriterInterface() noexcept(false) {}
  virtual void AddPoint(uint64_t id, double lat, double lon) = 0;
  virtual uint64_t GetNumProcessedPoints() const = 0;
};

class PointStorageReaderInterface
{
public:
  virtual ~PointStorageReaderInterface() = default;
  virtual bool GetPoint(uint64_t id, double & lat, double & lon) const = 0;
};

class IndexFileReader
{
public:
  using Value = uint64_t;

  IndexFileReader() = default;
  explicit IndexFileReader(std::string const & name);

  bool GetValueByKey(Key key, Value & value) const;

  template <typename ToDo>
  void ForEachByKey(Key k, ToDo && toDo) const
  {
    auto range = std::equal_range(m_elements.begin(), m_elements.end(), k, ElementComparator());
    for (; range.first != range.second; ++range.first)
      if (toDo((*range.first).second) == base::ControlFlow::Break)
        break;
  }

private:
  using Element = std::pair<Key, Value>;

  struct ElementComparator
  {
    bool operator()(Element const & r1, Element const & r2) const
    {
      return ((r1.first == r2.first) ? r1.second < r2.second : r1.first < r2.first);
    }
    bool operator()(Element const & r1, Key r2) const { return (r1.first < r2); }
    bool operator()(Key r1, Element const & r2) const { return (r1 < r2.first); }
  };

  std::vector<Element> m_elements;
};

class IndexFileWriter
{
public:
  using Value = uint64_t;

  explicit IndexFileWriter(std::string const & name);

  void WriteAll();
  void Add(Key k, Value const & v);

private:
  using Element = std::pair<Key, Value>;

  std::vector<Element> m_elements;
  FileWriter m_fileWriter;
};

class OSMElementCacheReaderInterface
{
public:
  virtual ~OSMElementCacheReaderInterface() = default;

  virtual bool Read(Key id, WayElement & value) = 0;
  virtual bool Read(Key id, RelationElement & value) = 0;
};

class IntermediateDataObjectsCache
{
public:
  // It's thread-safe class (All public methods are thread-safe).
  class AllocatedObjects
  {
  public:
    AllocatedObjects(feature::GenerateInfo::NodeStorageType type, std::string const & name);

    PointStorageReaderInterface const & GetPointStorageReader() const { return *m_storageReader; }

    IndexFileReader const & GetOrCreateIndexReader(std::string const & name);

  private:
    std::unique_ptr<PointStorageReaderInterface> m_storageReader;
    std::unordered_map<std::string, IndexFileReader> m_fileReaders;
  };

  // It's thread-safe method.
  AllocatedObjects & GetOrCreatePointStorageReader(feature::GenerateInfo::NodeStorageType type,
                                                   std::string const & name);

  void Clear();

private:
  std::mutex m_mutex;
  std::unordered_map<std::string, AllocatedObjects> m_objects;
};

class OSMElementCacheReader : public OSMElementCacheReaderInterface
{
public:
  explicit OSMElementCacheReader(IntermediateDataObjectsCache::AllocatedObjects & allocatedObjects,
                                 std::string const & name, bool preload = false);

  // OSMElementCacheReaderInterface overrides:
  bool Read(Key id, WayElement & value) override { return Read<>(id, value); }
  bool Read(Key id, RelationElement & value) override { return Read<>(id, value); }

private:
  template <class Value>
  bool Read(Key id, Value & value)
  {
    uint64_t pos = 0;
    if (!m_offsetsReader.GetValueByKey(id, pos))
    {
      LOG_SHORT(LWARNING, ("Can't find offset in file", m_name + OFFSET_EXT, "by id", id));
      return false;
    }

    uint32_t valueSize = m_preload ? *(reinterpret_cast<uint32_t *>(m_data.data() + pos)) : 0;
    size_t offset = pos + sizeof(uint32_t);

    if (!m_preload)
    {
      // in case not-in-memory work we read buffer
      m_fileReader.Read(pos, &valueSize, sizeof(valueSize));
      m_data.resize(valueSize);
      m_fileReader.Read(pos + sizeof(valueSize), m_data.data(), valueSize);
      offset = 0;
    }

    MemReader reader(m_data.data() + offset, valueSize);
    value.Read(reader);
    return true;
  }

  FileReader m_fileReader;
  IndexFileReader const & m_offsetsReader;
  std::string m_name;
  std::vector<uint8_t> m_data;
  bool m_preload = false;
};

class OSMElementCacheWriter
{
public:
  explicit OSMElementCacheWriter(std::string const & name);

  template <typename Value>
  void Write(Key id, Value const & value)
  {
    m_offsets.Add(id, m_fileWriter.Pos());
    m_data.clear();
    MemWriter<decltype(m_data)> w(m_data);

    value.Write(w);

    ASSERT_LESS(m_data.size(), std::numeric_limits<uint32_t>::max(), ());
    uint32_t sz = static_cast<uint32_t>(m_data.size());
    m_fileWriter.Write(&sz, sizeof(sz));
    m_fileWriter.Write(m_data.data(), sz);
  }

  void SaveOffsets();

private:
  FileWriter m_fileWriter;
  IndexFileWriter m_offsets;
  std::string m_name;
  std::vector<uint8_t> m_data;
};

class IntermediateDataReaderInterface
{
public:
  using ForEachRelationFn = std::function<base::ControlFlow(uint64_t, OSMElementCacheReaderInterface &)>;

  virtual ~IntermediateDataReaderInterface() = default;

  /// \a x \a y are in mercator projection coordinates. @see IntermediateDataWriter::AddNode.
  virtual bool GetNode(Key id, double & y, double & x) const = 0;
  virtual bool GetWay(Key id, WayElement & e) = 0;
  virtual bool GetRelation(Key id, RelationElement & e) = 0;

  virtual void ForEachRelationByNodeCached(Key /* id */, ForEachRelationFn & /* toDo */) {}
  virtual void ForEachRelationByWayCached(Key /* id */, ForEachRelationFn & /* toDo */) {}
  virtual void ForEachRelationByRelationCached(Key /* id */, ForEachRelationFn & /* toDo */) {}
};

class IntermediateDataReader : public IntermediateDataReaderInterface
{
public:
  // Constructs IntermediateDataReader.
  // objs - intermediate allocated objects. It's used for control allocated objects.
  // info - information about the generation.
  IntermediateDataReader(IntermediateDataObjectsCache::AllocatedObjects & objs, feature::GenerateInfo const & info);

  /// \a x \a y are in mercator projection coordinates. @see IntermediateDataWriter::AddNode.
  bool GetNode(Key id, double & y, double & x) const override { return m_nodes.GetPoint(id, y, x); }

  bool GetWay(Key id, WayElement & e) override { return m_ways.Read(id, e); }
  bool GetRelation(Key id, RelationElement & e) override { return m_relations.Read(id, e); }

  void ForEachRelationByWayCached(Key id, ForEachRelationFn & toDo) override
  {
    CachedRelationProcessor<ForEachRelationFn> processor(m_relations, toDo);
    m_wayToRelations.ForEachByKey(id, processor);
  }

  void ForEachRelationByNodeCached(Key id, ForEachRelationFn & toDo) override
  {
    CachedRelationProcessor<ForEachRelationFn> processor(m_relations, toDo);
    m_nodeToRelations.ForEachByKey(id, processor);
  }

  void ForEachRelationByRelationCached(Key id, ForEachRelationFn & toDo) override
  {
    CachedRelationProcessor<ForEachRelationFn> processor(m_relations, toDo);
    m_relationToRelations.ForEachByKey(id, processor);
  }

private:
  using CacheReader = cache::OSMElementCacheReader;

  template <typename Element, typename ToDo>
  class ElementProcessorBase
  {
  public:
    ElementProcessorBase(CacheReader & reader, ToDo & toDo) : m_reader(reader), m_toDo(toDo) {}

    base::ControlFlow operator()(uint64_t id)
    {
      Element e;
      return m_reader.Read(id, e) ? m_toDo(id, e) : base::ControlFlow::Break;
    }

  protected:
    CacheReader & m_reader;
    ToDo & m_toDo;
  };

  template <typename ToDo>
  struct CachedRelationProcessor : public ElementProcessorBase<RelationElement, ToDo>
  {
    using Base = ElementProcessorBase<RelationElement, ToDo>;

    CachedRelationProcessor(CacheReader & reader, ToDo & toDo) : Base(reader, toDo) {}

    base::ControlFlow operator()(uint64_t id) { return this->m_toDo(id, this->m_reader); }
  };

  PointStorageReaderInterface const & m_nodes;
  cache::OSMElementCacheReader m_ways;
  cache::OSMElementCacheReader m_relations;
  cache::IndexFileReader const & m_nodeToRelations;
  cache::IndexFileReader const & m_wayToRelations;
  cache::IndexFileReader const & m_relationToRelations;
};

class IntermediateDataWriter
{
public:
  IntermediateDataWriter(PointStorageWriterInterface & nodes, feature::GenerateInfo const & info);

  /// \a x \a y are in mercator projection coordinates. @see IntermediateDataReaderInterface::GetNode.
  void AddNode(Key id, double y, double x) { m_nodes.AddPoint(id, y, x); }
  void AddWay(Key id, WayElement const & e) { m_ways.Write(id, e); }

  void AddRelation(Key id, RelationElement const & e);
  void SaveIndex();

  static void AddToIndex(cache::IndexFileWriter & index, Key relationId, std::vector<uint64_t> const & values)
  {
    for (auto const v : values)
      index.Add(v, relationId);
  }

private:
  template <typename Container>
  static void AddToIndex(cache::IndexFileWriter & index, Key relationId, Container const & values)
  {
    for (auto const & v : values)
      index.Add(v.first, relationId);
  }

  PointStorageWriterInterface & m_nodes;
  cache::OSMElementCacheWriter m_ways;
  cache::OSMElementCacheWriter m_relations;
  cache::IndexFileWriter m_nodeToRelations;
  cache::IndexFileWriter m_wayToRelations;
  cache::IndexFileWriter m_relationToRelations;
};

std::unique_ptr<PointStorageReaderInterface> CreatePointStorageReader(feature::GenerateInfo::NodeStorageType type,
                                                                      std::string const & name);

std::unique_ptr<PointStorageWriterInterface> CreatePointStorageWriter(feature::GenerateInfo::NodeStorageType type,
                                                                      std::string const & name);

class IntermediateData
{
public:
  explicit IntermediateData(IntermediateDataObjectsCache & objectsCache, feature::GenerateInfo const & info);
  std::shared_ptr<IntermediateDataReader> const & GetCache() const;
  std::shared_ptr<IntermediateData> Clone() const;

private:
  IntermediateDataObjectsCache & m_objectsCache;
  feature::GenerateInfo const & m_info;
  std::shared_ptr<IntermediateDataReader> m_reader;

  DISALLOW_COPY(IntermediateData);
};
}  // namespace cache
}  // namespace generator
