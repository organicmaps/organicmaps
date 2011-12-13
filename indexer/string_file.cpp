#include "string_file.hpp"

#include "../coding/read_write_utils.hpp"
#include "../coding/reader.hpp"
#include "../coding/writer.hpp"

#include "../std/algorithm.hpp"


template <class TWriter>
StringsFile::IdT StringsFile::StringT::Write(TWriter & writer) const
{
  IdT const pos = static_cast<IdT>(writer.Pos());
  CHECK_EQUAL(static_cast<uint64_t>(pos), writer.Pos(), ());

  rw::Write(writer, m_name);
  WriteVarUint(writer, m_pos);
  WriteVarUint(writer, m_rank);

  return pos;
}

template <class TReader>
void StringsFile::StringT::Read(IdT id, TReader & reader)
{
  ReaderSource<TReader> src(reader);
  src.Skip(id);

  rw::Read(src, m_name);
  m_pos = ReadVarUint<uint32_t>(src);
  m_rank = ReadPrimitiveFromSource<uint8_t>(src);
}

bool StringsFile::StringT::operator<(StringT const & name) const
{
  if (m_name != name.m_name)
    return m_name < name.m_name;
  if (GetRank() != name.GetRank())
    return GetRank() > name.GetRank();
  if (GetOffset() != name.GetOffset())
    return GetOffset() < name.GetOffset();
  return false;
}

bool StringsFile::StringT::operator==(StringT const & name) const
{
  return (m_name == name.m_name && m_pos == name.m_pos && m_rank == name.m_rank);
}

void StringsFile::AddString(StringT const & s)
{
  ASSERT ( m_writer != 0, () );
  m_ids.push_back(s.Write(*m_writer));
}

bool StringsFile::StringCompare::operator() (IdT const & id1, IdT const & id2) const
{
  StringT str[2];
  str[0].Read(id1, m_file.m_reader);
  str[1].Read(id2, m_file.m_reader);
  return (str[0] < str[1]);
}

void StringsFile::SortStrings()
{
  stable_sort(m_ids.begin(), m_ids.end(), StringCompare(*this));
}

StringsFile::StringT StringsFile::IteratorT::dereference() const
{
  ASSERT_LESS ( m_index, m_file->m_ids.size(), () );

  StringT s;
  s.Read(m_file->m_ids[m_index], m_file->m_reader);
  return s;
}
