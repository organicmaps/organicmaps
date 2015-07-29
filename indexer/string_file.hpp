#pragma once

#include "coding/file_writer.hpp"
#include "coding/file_reader.hpp"

#include "base/macros.hpp"
#include "base/mem_trie.hpp"
#include "base/string_utils.hpp"
#include "base/worker_thread.hpp"

#include "coding/read_write_utils.hpp"
#include "std/iterator_facade.hpp"
#include "std/queue.hpp"
#include "std/functional.hpp"
#include "std/unique_ptr.hpp"

template <typename TValue>
class StringsFile
{
public:
  using ValueT = TValue;
  using IdT = uint32_t;

  class TString
  {
    strings::UniString m_name;
    ValueT m_val;

  public:
    TString() {}
    TString(strings::UniString const & name, signed char lang, ValueT const & val) : m_val(val)
    {
      m_name.reserve(name.size() + 1);
      m_name.push_back(static_cast<uint8_t>(lang));
      m_name.append(name.begin(), name.end());
    }

    TString(strings::UniString const & langName, ValueT const & val) : m_name(langName), m_val(val)
    {
    }

    uint32_t GetKeySize() const { return m_name.size(); }
    uint32_t const * GetKeyData() const { return m_name.data(); }

    strings::UniString const & GetString() const { return m_name; }

    ValueT const & GetValue() const { return m_val; }

    bool operator<(TString const & name) const
    {
      if (m_name != name.m_name)
        return m_name < name.m_name;
      return m_val < name.m_val;
    }

    bool operator==(TString const & name) const
    {
      return m_name == name.m_name && m_val == name.m_val;
    }

    template <class TWriter>
    IdT Write(TWriter & writer) const
    {
      IdT const pos = static_cast<IdT>(writer.Pos());
      CHECK_EQUAL(static_cast<uint64_t>(pos), writer.Pos(), ());

      rw::Write(writer, m_name);
      m_val.Write(writer);

      return pos;
    }

    template <class TReader>
    void Read(TReader & src)
    {
      rw::Read(src, m_name);
      m_val.Read(src);
    }

    inline void const * value_data() const { return m_val.data(); }

    inline size_t value_size() const { return m_val.size(); }

    void Swap(TString & r)
    {
      m_name.swap(r.m_name);
      m_val.swap(r.m_val);
    }
  };

  using StringsListT = vector<TString>;

  // Contains start and end offsets of file portions.
  using OffsetsListT = vector<pair<uint64_t, uint64_t>>;

  /// This class encapsulates a task to efficiently sort a bunch of
  /// strings and writes them in a sorted oreder.
  class SortAndDumpStringsTask
  {
  public:
    /// A class ctor.
    ///
    /// \param writer A writer that will be used to write strings.
    /// \param offsets A list of offsets [begin, end) that denote
    ///                groups of sorted strings in a file.  When strings will be
    ///                sorted and dumped on a disk, a pair of offsets will be added
    ///                to the list.
    /// \param strings Vector of strings that should be sorted. Internal data is moved out from
    ///                strings, so it'll become empty after ctor.
    SortAndDumpStringsTask(FileWriter & writer, OffsetsListT & offsets, StringsListT & strings)
        : m_writer(writer), m_offsets(offsets)
    {
      strings.swap(m_strings);
    }

    /// Sorts strings via in-memory trie and writes them.
    void operator()()
    {
      vector<uint8_t> memBuffer;
      {
        my::MemTrie<strings::UniString, ValueT> trie;
        for (auto const & s : m_strings)
          trie.Add(s.GetString(), s.GetValue());
        MemWriter<vector<uint8_t>> memWriter(memBuffer);
        trie.ForEach([&memWriter](const strings::UniString & s, const ValueT & v)
                     {
                       rw::Write(memWriter, s);
                       v.Write(memWriter);
                     });
      }

      uint64_t const spos = m_writer.Pos();
      m_writer.Write(memBuffer.data(), memBuffer.size());
      uint64_t const epos = m_writer.Pos();
      m_offsets.push_back(make_pair(spos, epos));
      m_writer.Flush();
    }

  private:
    FileWriter & m_writer;
    OffsetsListT & m_offsets;
    StringsListT m_strings;

    DISALLOW_COPY_AND_MOVE(SortAndDumpStringsTask);
  };

  class IteratorT : public iterator_facade<IteratorT, TString, forward_traversal_tag, TString>
  {
    StringsFile & m_file;
    bool m_end;

    bool IsEnd() const;
    inline bool IsValid() const { return (!m_end && !IsEnd()); }

  public:
    IteratorT(StringsFile & file, bool isEnd) : m_file(file), m_end(isEnd)
    {
      // Additional check in case for empty sequence.
      if (!m_end)
        m_end = IsEnd();
    }

    TString dereference() const;
    bool equal(IteratorT const & r) const { return (m_end == r.m_end); }
    void increment();
  };

  StringsFile(string const & fPath);

  void EndAdding();
  void OpenForRead();

  /// @precondition Should be opened for writing.
  void AddString(TString const & s);

  IteratorT Begin() { return IteratorT(*this, false); }
  IteratorT End() { return IteratorT(*this, true); }

private:
  unique_ptr<FileWriter> m_writer;
  unique_ptr<FileReader> m_reader;

  void Flush();
  bool PushNextValue(size_t i);

  StringsListT m_strings;
  OffsetsListT m_offsets;

  // A worker thread that sorts and writes groups of strings.  The
  // whole process looks like a pipeline, i.e. main thread accumulates
  // strings while worker thread sequentially sorts and stores groups
  // of strings on a disk.
  my::WorkerThread<SortAndDumpStringsTask> m_workerThread;

  struct QValue
  {
    TString m_string;
    size_t m_index;

    QValue(TString const & s, size_t i) : m_string(s), m_index(i) {}

    inline bool operator>(QValue const & rhs) const { return !(m_string < rhs.m_string); }
  };

  priority_queue<QValue, vector<QValue>, greater<QValue>> m_queue;
};

template <typename ValueT>
void StringsFile<ValueT>::AddString(TString const & s)
{
  size_t const kMaxSize = 1000000;

  if (m_strings.size() >= kMaxSize)
    Flush();

  m_strings.push_back(s);
}

template <typename ValueT>
bool StringsFile<ValueT>::IteratorT::IsEnd() const
{
  return m_file.m_queue.empty();
}

template <typename ValueT>
typename StringsFile<ValueT>::TString StringsFile<ValueT>::IteratorT::dereference() const
{
  ASSERT(IsValid(), ());
  return m_file.m_queue.top().m_string;
}

template <typename ValueT>
void StringsFile<ValueT>::IteratorT::increment()
{
  ASSERT(IsValid(), ());
  int const index = m_file.m_queue.top().m_index;

  m_file.m_queue.pop();

  if (!m_file.PushNextValue(index))
    m_end = IsEnd();
}

template <typename ValueT>
StringsFile<ValueT>::StringsFile(string const & fPath)
    : m_workerThread(1 /* maxTasks */)
{
  m_writer.reset(new FileWriter(fPath));
}

template <typename ValueT>
void StringsFile<ValueT>::Flush()
{
  shared_ptr<SortAndDumpStringsTask> task(
      new SortAndDumpStringsTask(*m_writer, m_offsets, m_strings));
  m_workerThread.Push(task);
}

template <typename ValueT>
bool StringsFile<ValueT>::PushNextValue(size_t i)
{
  // reach the end of the portion file
  if (m_offsets[i].first >= m_offsets[i].second)
    return false;

  // init source to needed offset
  ReaderSource<FileReader> src(*m_reader);
  src.Skip(m_offsets[i].first);

  // read string
  TString s;
  s.Read(src);

  // update offset
  m_offsets[i].first = src.Pos();

  // push value to queue
  m_queue.push(QValue(s, i));
  return true;
}

template <typename ValueT>
void StringsFile<ValueT>::EndAdding()
{
  Flush();

  m_workerThread.RunUntilIdleAndStop();

  m_writer->Flush();
}

template <typename ValueT>
void StringsFile<ValueT>::OpenForRead()
{
  string const fPath = m_writer->GetName();
  m_writer.reset();

  m_reader.reset(new FileReader(fPath));

  for (size_t i = 0; i < m_offsets.size(); ++i)
    PushNextValue(i);
}
