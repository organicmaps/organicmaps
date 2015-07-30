#pragma once

#include "generator/osm_decl.hpp"

#include "coding/file_reader_stream.hpp"
#include "coding/file_writer_stream.hpp"
#include "coding/file_name_utils.hpp"

#include "base/logging.hpp"

#include "std/utility.hpp"
#include "std/vector.hpp"
#include "std/algorithm.hpp"
#include "std/limits.hpp"
#include "std/exception.hpp"

/// Classes for reading and writing any data in file with map of offsets for
/// fast searching in memory by some key.
namespace cache
{
namespace detail
{
template <class TFile, class TValue>
class IndexFile
{
  using TKey = uint64_t;
  static_assert(is_integral<TKey>::value, "TKey is not integral type");
  using TElement = pair<TKey, TValue>;
  using TContainer = vector<TElement>;

  TContainer m_elements;
  TFile m_file;

  static size_t constexpr kFlushCount = 1024;

  struct ElementComparator
  {
    bool operator()(TElement const & r1, TElement const & r2) const
    {
      return ((r1.first == r2.first) ? r1.second < r2.second : r1.first < r2.first);
    }
    bool operator()(TElement const & r1, TKey r2) const { return (r1.first < r2); }
    bool operator()(TKey r1, TElement const & r2) const { return (r1 < r2.first); }
  };

  static size_t CheckedCast(uint64_t v)
  {
    ASSERT_LESS(v, numeric_limits<size_t>::max(), ("Value too long for memory address : ", v));
    return static_cast<size_t>(v);
  }

public:
  IndexFile(string const & name) : m_file(name.c_str()) {}

  string GetFileName() const { return m_file.GetName(); }

  void Flush()
  {
    if (m_elements.empty())
      return;

    m_file.Write(&m_elements[0], m_elements.size() * sizeof(TElement));
    m_elements.clear();
  }

  void ReadAll()
  {
    m_elements.clear();
    uint64_t const fileSize = m_file.Size();
    if (fileSize == 0)
      return;

    LOG_SHORT(LINFO, ("Offsets reading is started for file ", GetFileName()));
    CHECK_EQUAL(0, fileSize % sizeof(TElement), ("Damaged file."));

    try
    {
      m_elements.resize(CheckedCast(fileSize / sizeof(TElement)));
    }
    catch (exception const &)  // bad_alloc
    {
      LOG(LCRITICAL, ("Insufficient memory for required offset map"));
    }

    m_file.Read(0, &m_elements[0], CheckedCast(fileSize));

    sort(m_elements.begin(), m_elements.end(), ElementComparator());

    LOG_SHORT(LINFO, ("Offsets reading is finished"));
  }

  void Write(TKey k, TValue const & v)
  {
    if (m_elements.size() > kFlushCount)
      Flush();

    m_elements.push_back(make_pair(k, v));
  }

  bool GetValueByKey(TKey k, TValue & v) const
  {
    auto it = lower_bound(m_elements.begin(), m_elements.end(), k, ElementComparator());
    if ((it != m_elements.end()) && ((*it).first == k))
    {
      v = (*it).second;
      return true;
    }
    return false;
  }

  template <class ToDo>
  void ForEachByKey(TKey k, ToDo && toDo) const
  {
    auto range = equal_range(m_elements.begin(), m_elements.end(), k, ElementComparator());
    for (; range.first != range.second; ++range.first)
    {
      if (toDo((*range.first).second))
        return;
    }
  }
};
} // namespace detail

template <class TStream, class TOffsetFile>
class DataFileBase
{
public:
  using TKey = uint64_t;

protected:
  TStream m_stream;
  detail::IndexFile<TOffsetFile, uint64_t> m_offsets;

public:
  DataFileBase(string const & name) : m_stream(name), m_offsets(name + OFFSET_EXT) {}
};

class DataFileWriter : public DataFileBase<FileWriterStream, FileWriter>
{
  using TBase = DataFileBase<FileWriterStream, FileWriter>;

public:
  DataFileWriter(string const & name) : TBase(name) {}

  template <class TValue>
  void Write(TKey id, TValue const & value)
  {
    m_offsets.Write(id, m_stream.Pos());
    m_stream << value;
  }

  void SaveOffsets() { m_offsets.Flush(); }
};

class DataFileReader : public DataFileBase<FileReaderStream, FileReader>
{
  using TBase = DataFileBase<FileReaderStream, FileReader>;

public:
  DataFileReader(string const & name) : TBase(name) {}

  template <class TValue>
  bool Read(TKey id, TValue & value)
  {
    uint64_t pos;
    if (m_offsets.GetValueByKey(id, pos))
    {
      m_stream.Seek(pos);
      m_stream >> value;
      return true;
    }
    else
    {
      LOG_SHORT(LWARNING, ("Can't find offset in file ", m_offsets.GetFileName(), " by id ", id));
      return false;
    }
  }

  void LoadOffsets() { m_offsets.ReadAll(); }
};

template <class TNodesHolder, class TData, class TFile>
class BaseFileHolder
{
protected:
  using TKey = typename TData::TKey;
  static_assert(is_integral<TKey>::value, "TKey is not integral type");

  using TIndex = detail::IndexFile<TFile, TKey>;

  TNodesHolder & m_nodes;

  TData m_ways;
  TData m_relations;

  TIndex m_nodes2rel;
  TIndex m_ways2rel;

public:
  BaseFileHolder(TNodesHolder & nodes, string const & dir)
    : m_nodes(nodes)
    , m_ways(my::JoinFoldersToPath(dir, WAYS_FILE))
    , m_relations(my::JoinFoldersToPath(dir, RELATIONS_FILE))
    , m_nodes2rel(my::JoinFoldersToPath(dir, string(NODES_FILE) + ID2REL_EXT))
    , m_ways2rel(my::JoinFoldersToPath(dir, string(WAYS_FILE) + ID2REL_EXT))
  {
  }
};
}
