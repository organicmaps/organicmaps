#pragma once

#include "generator/generate_info.hpp"
#include "generator/intermediate_elements.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/mmap_reader.hpp"

#include "base/logging.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "defines.hpp"

// Classes for reading and writing any data in file with map of offsets for
// fast searching in memory by some key.
namespace generator
{
namespace cache
{
using Key = uint64_t;

// Used to store all world nodes inside temporary index file.
// To find node by id, just calculate offset inside index file:
// offset_in_file = sizeof(LatLon) * node_ID
struct LatLon
{
  int32_t lat;
  int32_t lon;
};
static_assert(sizeof(LatLon) == 8, "Invalid structure size");

struct LatLonPos
{
  uint64_t pos;
  int32_t lat;
  int32_t lon;
};
static_assert(sizeof(LatLonPos) == 16, "Invalid structure size");

class IndexFileReader
{
public:
  using Value = uint64_t;

  explicit IndexFileReader(std::string const & name);

  void ReadAll();

  bool GetValueByKey(Key key, Value & value) const;

  template <typename ToDo>
  void ForEachByKey(Key k, ToDo && toDo) const
  {
    auto range = std::equal_range(m_elements.begin(), m_elements.end(), k, ElementComparator());
    for (; range.first != range.second; ++range.first)
    {
      if (toDo((*range.first).second))
        return;
    }
  }

private:
  static_assert(is_integral<Key>::value, "Key must be an integral type");
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
  FileReader m_fileReader;
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

class OSMElementCacheReader
{
public:
  OSMElementCacheReader(std::string const & name, bool preload = false);

  template <class Value>
  bool Read(Key id, Value & value)
  {
    uint64_t pos = 0;
    if (!m_offsets.GetValueByKey(id, pos))
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

  void LoadOffsets();

protected:
  FileReader m_fileReader;
  IndexFileReader m_offsets;
  std::string m_name;
  std::vector<uint8_t> m_data;
  bool m_preload = false;
};

class OSMElementCacheWriter
{
public:
  OSMElementCacheWriter(std::string const & name, bool preload = false);

  template <typename Value>
  void Write(Key id, Value const & value)
  {
    m_offsets.Add(id, m_fileWriter.Pos());
    m_data.clear();
    MemWriter<decltype(m_data)> w(m_data);

    value.Write(w);

    ASSERT_LESS(m_data.size(), std::numeric_limits<uint32_t>::max(), ());
    uint32_t sz = static_cast<uint32_t>(m_data.size());
    // ?!
    m_fileWriter.Write(&sz, sizeof(sz));
    m_fileWriter.Write(m_data.data(), sz);
  }

  void SaveOffsets();

protected:
  FileWriter m_fileWriter;
  IndexFileWriter m_offsets;
  std::string m_name;
  std::vector<uint8_t> m_data;
  bool m_preload = false;
};

class RawFilePointStorageMmapReader
{
public:
  explicit RawFilePointStorageMmapReader(std::string const & name);

  bool GetPoint(uint64_t id, double & lat, double & lon) const;

private:
  MmapReader m_mmapReader;
};

class RawFilePointStorageWriter
{
public:
  explicit RawFilePointStorageWriter(std::string const & name);

  void AddPoint(uint64_t id, double lat, double lon);
  uint64_t GetNumProcessedPoints() { return m_numProcessedPoints; }

private:
  FileWriter m_fileWriter;
  uint64_t m_numProcessedPoints = 0;
};

class RawMemPointStorageReader
{
public:
  explicit RawMemPointStorageReader(std::string const & name);

  bool GetPoint(uint64_t id, double & lat, double & lon) const;

private:
  FileReader m_fileReader;
  std::vector<LatLon> m_data;
};

class RawMemPointStorageWriter
{
public:
  explicit RawMemPointStorageWriter(std::string const & name);

  ~RawMemPointStorageWriter();

  void AddPoint(uint64_t id, double lat, double lon);
  uint64_t GetNumProcessedPoints() { return m_numProcessedPoints; }

private:
  FileWriter m_fileWriter;
  std::vector<LatLon> m_data;
  uint64_t m_numProcessedPoints = 0;
};

class MapFilePointStorageReader
{
public:
  explicit MapFilePointStorageReader(std::string const & name);

  bool GetPoint(uint64_t id, double & lat, double & lon) const;

private:
  FileReader m_fileReader;
  std::unordered_map<uint64_t, std::pair<int32_t, int32_t>> m_map;
};

class MapFilePointStorageWriter
{
public:
  explicit MapFilePointStorageWriter(std::string const & name);

  void AddPoint(uint64_t id, double lat, double lon);
  uint64_t GetNumProcessedPoints() { return m_numProcessedPoints; }

private:
  FileWriter m_fileWriter;
  std::unordered_map<uint64_t, std::pair<int32_t, int32_t>> m_map;
  uint64_t m_numProcessedPoints = 0;
};

template <typename NodesHolder>
class IntermediateDataReader
{
public:
  IntermediateDataReader(NodesHolder & nodes, feature::GenerateInfo & info)
    : m_nodes(nodes)
    , m_ways(info.GetIntermediateFileName(WAYS_FILE, ""), info.m_preloadCache)
    , m_relations(info.GetIntermediateFileName(RELATIONS_FILE, ""), info.m_preloadCache)
    , m_nodeToRelations(info.GetIntermediateFileName(NODES_FILE, ID2REL_EXT))
    , m_wayToRelations(info.GetIntermediateFileName(WAYS_FILE, ID2REL_EXT))
  {
  }

  bool GetNode(Key id, double & lat, double & lon) { return m_nodes.GetPoint(id, lat, lon); }
  bool GetWay(Key id, WayElement & e) { return m_ways.Read(id, e); }

  template <class ToDo>
  void ForEachRelationByWay(Key id, ToDo && toDo)
  {
    RelationProcessor<ToDo> processor(m_relations, toDo);
    m_wayToRelations.ForEachByKey(id, processor);
  }

  template <class ToDo>
  void ForEachRelationByWayCached(Key id, ToDo && toDo)
  {
    CachedRelationProcessor<ToDo> processor(m_relations, toDo);
    m_wayToRelations.ForEachByKey(id, processor);
  }

  template <class ToDo>
  void ForEachRelationByNodeCached(Key id, ToDo && toDo)
  {
    CachedRelationProcessor<ToDo> processor(m_relations, toDo);
    m_nodeToRelations.ForEachByKey(id, processor);
  }

  void LoadIndex()
  {
    m_ways.LoadOffsets();
    m_relations.LoadOffsets();

    m_nodeToRelations.ReadAll();
    m_wayToRelations.ReadAll();
  }

private:
  using CacheReader = cache::OSMElementCacheReader;

  template <class Element, class ToDo>
  struct ElementProcessorBase
  {
  protected:
    CacheReader m_reader;
    ToDo & m_toDo;

  public:
    ElementProcessorBase(CacheReader & reader, ToDo & toDo) : m_reader(reader), m_toDo(toDo) {}

    bool operator()(uint64_t id)
    {
      Element e;
      return m_reader.Read(id, e) ? m_toDo(id, e) : false;
    }
  };

  template <class ToDo>
  struct RelationProcessor : public ElementProcessorBase<RelationElement, ToDo>
  {
    using Base = ElementProcessorBase<RelationElement, ToDo>;

    RelationProcessor(CacheReader & reader, ToDo & toDo) : Base(reader, toDo) {}
  };

  template <class ToDo>
  struct CachedRelationProcessor : public RelationProcessor<ToDo>
  {
    using Base = RelationProcessor<ToDo>;

    CachedRelationProcessor(CacheReader & reader, ToDo & toDo) : Base(reader, toDo) {}
    bool operator()(uint64_t id) { return this->m_toDo(id, this->m_reader); }
  };

  NodesHolder & m_nodes;

  cache::OSMElementCacheReader m_ways;
  cache::OSMElementCacheReader m_relations;

  cache::IndexFileReader m_nodeToRelations;
  cache::IndexFileReader m_wayToRelations;
};

template <typename NodesHolder>
class IntermediateDataWriter
{
public:
  IntermediateDataWriter(NodesHolder & nodes, feature::GenerateInfo & info)
    : m_nodes(nodes)
    , m_ways(info.GetIntermediateFileName(WAYS_FILE, ""), info.m_preloadCache)
    , m_relations(info.GetIntermediateFileName(RELATIONS_FILE, ""), info.m_preloadCache)
    , m_nodeToRelations(info.GetIntermediateFileName(NODES_FILE, ID2REL_EXT))
    , m_wayToRelations(info.GetIntermediateFileName(WAYS_FILE, ID2REL_EXT))
  {
  }

  void AddNode(Key id, double lat, double lon) { m_nodes.AddPoint(id, lat, lon); }

  void AddWay(Key id, WayElement const & e) { m_ways.Write(id, e); }

  void AddRelation(Key id, RelationElement const & e)
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

  void SaveIndex()
  {
    m_ways.SaveOffsets();
    m_relations.SaveOffsets();

    m_nodeToRelations.WriteAll();
    m_wayToRelations.WriteAll();
  }

private:
  NodesHolder & m_nodes;

  cache::OSMElementCacheWriter m_ways;
  cache::OSMElementCacheWriter m_relations;

  cache::IndexFileWriter m_nodeToRelations;
  cache::IndexFileWriter m_wayToRelations;

  template <class Index, class Container>
  static void AddToIndex(Index & index, Key relationId, Container const & values)
  {
    for (auto const & v : values)
      index.Add(v.first, relationId);
  }
};
}  // namespace cache
}  // namespace generator
