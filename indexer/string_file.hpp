#pragma once

#include "../coding/file_writer.hpp"
#include "../coding/file_reader.hpp"

#include "../base/macros.hpp"
#include "../base/mem_trie.hpp"
#include "../base/string_utils.hpp"
#include "../base/worker_thread.hpp"

#include "../coding/read_write_utils.hpp"
#include "../std/iterator_facade.hpp"
#include "../std/queue.hpp"
#include "../std/functional.hpp"
#include "../std/unique_ptr.hpp"


class StringsFile
{
public:
  typedef buffer_vector<uint8_t, 32> ValueT;
  typedef uint32_t IdT;

  class StringT
  {
    strings::UniString m_name;
    ValueT m_val;

  public:
    StringT() {}
    StringT(strings::UniString const & name, signed char lang, ValueT const & val)
      : m_val(val)
    {
      m_name.reserve(name.size() + 1);
      m_name.push_back(static_cast<uint8_t>(lang));
      m_name.append(name.begin(), name.end());
    }

    uint32_t GetKeySize() const { return m_name.size(); }
    uint32_t const * GetKeyData() const { return m_name.data(); }

    strings::UniString const & GetString() const { return m_name; }

    ValueT const & GetValue() const { return m_val; }

    template <class TCont> void SerializeValue(TCont & cont) const
    {
      cont.assign(m_val.begin(), m_val.end());
    }

    bool operator < (StringT const & name) const;
    bool operator == (StringT const & name) const;

    template <class TWriter> IdT Write(TWriter & writer) const;
    template <class TReader> void Read(TReader & src);

    void Swap(StringT & r)
    {
      m_name.swap(r.m_name);
      m_val.swap(r.m_val);
    }
  };

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
    SortAndDumpStringsTask(FileWriter & writer, vector<pair<uint64_t, uint64_t>> & offsets,
                           std::vector<StringsFile::StringT> & strings)
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
                       rw::WriteVectorOfPOD(memWriter, v);
                     });
      }

      uint64_t const spos = m_writer.Pos();
      m_writer.Write(&memBuffer[0], memBuffer.size());
      uint64_t const epos = m_writer.Pos();
      m_offsets.push_back(make_pair(spos, epos));
      m_writer.Flush();
    }

  private:
    FileWriter & m_writer;
    vector<pair<uint64_t, uint64_t>> & m_offsets;
    vector<StringsFile::StringT> m_strings;

    DISALLOW_COPY_AND_MOVE(SortAndDumpStringsTask);
  };

  class IteratorT : public iterator_facade<IteratorT, StringT, forward_traversal_tag, StringT>
  {
    StringsFile & m_file;
    bool m_end;

    bool IsEnd() const;
    inline bool IsValid() const { return (!m_end && !IsEnd()); }

  public:
    IteratorT(StringsFile & file, bool isEnd)
      : m_file(file), m_end(isEnd)
    {
      // Additional check in case for empty sequence.
      if (!m_end)
        m_end = IsEnd();
    }

    StringT dereference() const;
    bool equal(IteratorT const & r) const { return (m_end == r.m_end); }
    void increment();
  };

  StringsFile(string const & fPath);

  void EndAdding();
  void OpenForRead();

  /// @precondition Should be opened for writing.
  void AddString(StringT const & s);

  IteratorT Begin() { return IteratorT(*this, false); }
  IteratorT End() { return IteratorT(*this, true); }

private:
  unique_ptr<FileWriter> m_writer;
  unique_ptr<FileReader> m_reader;

  void Flush();
  bool PushNextValue(size_t i);

  vector<StringT> m_strings;

  // Contains start and end offsets of file portions.
  vector<pair<uint64_t, uint64_t> > m_offsets;

  // A worker thread that sorts and writes groups of strings.  The
  // whole process looks like a pipeline, i.e. main thread accumulates
  // strings while worker thread sequentially sorts and stores groups
  // of strings on a disk.
  my::WorkerThread<SortAndDumpStringsTask> m_workerThread;

  struct QValue
  {
    StringT m_string;
    size_t m_index;

    QValue(StringT const & s, size_t i) : m_string(s), m_index(i) {}

    inline bool operator > (QValue const & rhs) const { return !(m_string < rhs.m_string); }
  };

  priority_queue<QValue, vector<QValue>, greater<QValue> > m_queue;
};
