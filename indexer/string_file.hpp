#pragma once

#include "../coding/reader.hpp"

#include "../base/string_utils.hpp"

#include "../std/iterator_facade.hpp"
#include "../std/queue.hpp"
#include "../std/functional.hpp"


class StringsFile
{
public:

  typedef uint32_t IdT;

  class StringT
  {
    strings::UniString m_name;
    uint32_t m_pos;
    uint8_t m_rank;

  public:
    StringT() {}
    StringT(strings::UniString const & name, signed char lang, uint32_t pos, uint8_t rank)
      : m_pos(pos), m_rank(rank)
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
      cont.resize(5);
      cont[0] = m_rank;
      uint32_t const i = SwapIfBigEndian(m_pos);
      memcpy(&cont[1], &i, 4);
    }

    uint8_t GetRank() const { return m_rank; }
    uint32_t GetOffset() const { return m_pos; }

    bool operator < (StringT const & name) const;
    bool operator == (StringT const & name) const;

    template <class TWriter> IdT Write(TWriter & writer) const;
    template <class TReader> void Read(TReader & src);

    void Swap(StringT & r)
    {
      m_name.swap(r.m_name);
      swap(m_pos, r.m_pos);
      swap(m_rank, r.m_rank);
    }
  };

  class IteratorT : public iterator_facade<IteratorT, StringT, forward_traversal_tag, StringT>
  {
    StringsFile & m_file;
    bool m_end;

  public:
    IteratorT(StringsFile & file, bool isEnd)
      : m_file(file), m_end(isEnd)
    {
    }

    StringT dereference() const;
    bool equal(IteratorT const & r) const { return m_end == r.m_end; }
    void increment();
  };

  StringsFile(string const & fPath) : m_filePath(fPath), m_index(0) {}
  ~StringsFile();

  void EndAdding();
  void OpenForRead();

  /// @precondition Should be opened for writing.
  void AddString(StringT const & s);

  IteratorT Begin() { return IteratorT(*this, false); }
  IteratorT End() { return IteratorT(*this, true); }

private:
  string FormatFilePath(int i) const;
  void Flush();
  bool PushNextValue(int i);

  vector<StringT> m_strings;
  string m_filePath;
  int m_index;

  typedef ReaderSource<ReaderPtr<Reader> > ReaderT;
  vector<ReaderT> m_readers;

  struct QValue
  {
    StringT m_string;
    int m_index;

    QValue(StringT const & s, int i) : m_string(s), m_index(i) {}

    inline bool operator > (QValue const & rhs) const { return !(m_string < rhs.m_string); }
  };

  priority_queue<QValue, vector<QValue>, greater<QValue> > m_queue;
};
