#pragma once

#include "generator/osm_decl.hpp"

#include "coding/file_name_utils.hpp"
#include "coding/file_reader_stream.hpp"
#include "coding/file_writer_stream.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"
#include "std/exception.hpp"
#include "std/limits.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

#include "std/fstream.hpp"

/// Classes for reading and writing any data in file with map of offsets for
/// fast searching in memory by some key.
namespace cache
{

enum class EMode { Write = true, Read = false };

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

  void WriteAll()
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

  void Add(TKey k, TValue const & v)
  {
    if (m_elements.size() > kFlushCount)
      WriteAll();

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


template <EMode TMode>
class OSMElementCache
{
public:
  using TKey = uint64_t;
  using TStream = typename conditional<TMode == EMode::Write, FileWriterStream, FileReaderStream>::type;
  using TOffsetFile = typename conditional<TMode == EMode::Write, FileWriter, FileReader>::type;

protected:
  TStream m_stream;
  detail::IndexFile<TOffsetFile, uint64_t> m_offsets;
  string m_name;

public:
  OSMElementCache(string const & name) : m_stream(name), m_offsets(name + OFFSET_EXT), m_name(name) {}

  template <class TValue>
  void Write(TKey id, TValue const & value)
  {
    m_offsets.Add(id, m_stream.Pos());
    value.Write(m_stream);
    std::ofstream ff((m_name+".wlog").c_str(), std::ios::binary | std::ios::app);
    ff << id << " " << value.ToString() << std::endl;
  }

  template <class TValue>
  bool Read(TKey id, TValue & value)
  {
    uint64_t pos;
    if (m_offsets.GetValueByKey(id, pos))
    {
      m_stream.Seek(pos);
      value.Read(m_stream);
      std::ofstream ff((m_name+".rlog").c_str(), std::ios::binary | std::ios::app);
      ff << id << " " << value.ToString() << std::endl;
      return true;
    }
    else
    {
      LOG_SHORT(LWARNING, ("Can't find offset in file ", m_offsets.GetFileName(), " by id ", id));
      return false;
    }
  }

  inline void SaveOffsets() { m_offsets.WriteAll(); }
  inline void LoadOffsets() { m_offsets.ReadAll(); }
};
} // namespace cache
