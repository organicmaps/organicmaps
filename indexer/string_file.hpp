#pragma once

#include "../coding/writer.hpp"
#include "../coding/reader.hpp"

#include "../base/string_utils.hpp"

#include "../std/iterator_facade.hpp"


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
    template <class TReader> void Read(IdT id, TReader & reader);

    void Swap(StringT & r)
    {
      m_name.swap(r.m_name);
      swap(m_pos, r.m_pos);
      swap(m_rank, r.m_rank);
    }
  };

  class StringCompare
  {
    StringsFile & m_file;
  public:
    StringCompare(StringsFile & file) : m_file(file) {}
    bool operator() (IdT const & id1, IdT const & id2) const;
  };

  class IteratorT : public iterator_facade<IteratorT, StringT, forward_traversal_tag, StringT>
  {
    size_t m_index;
    StringsFile const * m_file;

  public:
    IteratorT(size_t index, StringsFile const & file)
      : m_index(index), m_file(&file) {}

    StringT dereference() const;
    bool equal(IteratorT const & r) const { return m_index == r.m_index; }
    void increment() { ++m_index; }
  };

  StringsFile() : m_writer(0), m_reader(0) {}

  void OpenForWrite(Writer * w) { m_writer = w; }
  /// Note! r should be in dynamic memory and this class takes shared ownership of it.
  void OpenForRead(Reader * r) { m_reader = ReaderPtr<Reader>(r); }

  /// @precondition Should be opened for writing.
  void AddString(StringT const & s);

  /// @precondition Should be opened for reading.
  void SortStrings();

  IteratorT Begin() const { return IteratorT(0, *this); }
  IteratorT End() const { return IteratorT(m_ids.size(), *this); }

private:
  vector<IdT> m_ids;

  Writer * m_writer;
  ReaderPtr<Reader> m_reader;
};
