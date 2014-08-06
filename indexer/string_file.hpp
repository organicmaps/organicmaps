#pragma once

#include "../coding/file_writer.hpp"
#include "../coding/file_reader.hpp"

#include "../base/string_utils.hpp"

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
  // store start and end offsets of file portions
  vector<pair<uint64_t, uint64_t> > m_offsets;

  struct QValue
  {
    StringT m_string;
    size_t m_index;

    QValue(StringT const & s, size_t i) : m_string(s), m_index(i) {}

    inline bool operator > (QValue const & rhs) const { return !(m_string < rhs.m_string); }
  };

  priority_queue<QValue, vector<QValue>, greater<QValue> > m_queue;
};
