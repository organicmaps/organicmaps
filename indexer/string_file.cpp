#include "string_file.hpp"

#include "../coding/read_write_utils.hpp"
#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"

#include "../base/logging.hpp"

#include "../std/algorithm.hpp"
#include "../std/bind.hpp"


template <class TWriter>
StringsFile::IdT StringsFile::StringT::Write(TWriter & writer) const
{
  IdT const pos = static_cast<IdT>(writer.Pos());
  CHECK_EQUAL(static_cast<uint64_t>(pos), writer.Pos(), ());

  rw::Write(writer, m_name);
  WriteVarUint(writer, m_pos);
  WriteToSink(writer, m_rank);

  return pos;
}

template <class TReader>
void StringsFile::StringT::Read(TReader & src)
{
  rw::Read(src, m_name);
  m_pos = ReadVarUint<uint32_t>(src);
  m_rank = ReadPrimitiveFromSource<uint8_t>(src);
}

bool StringsFile::StringT::operator < (StringT const & name) const
{
  if (m_name != name.m_name)
    return m_name < name.m_name;
  if (GetRank() != name.GetRank())
    return GetRank() > name.GetRank();
  if (GetOffset() != name.GetOffset())
    return GetOffset() < name.GetOffset();
  return false;
}

bool StringsFile::StringT::operator == (StringT const & name) const
{
  return (m_name == name.m_name && m_pos == name.m_pos && m_rank == name.m_rank);
}

StringsFile::~StringsFile()
{
  m_readers.clear();

  for (int i = 0; i < m_index; ++i)
    FileWriter::DeleteFileX(FormatFilePath(i));
}

void StringsFile::AddString(StringT const & s)
{
  if (m_strings.size() >= 30000)
    Flush();

  m_strings.push_back(s);
}

StringsFile::StringT StringsFile::IteratorT::dereference() const
{
  ASSERT ( !m_file.m_queue.empty(), () );
  return m_file.m_queue.top().m_string;
}

void StringsFile::IteratorT::increment()
{
  ASSERT ( !m_file.m_queue.empty(), () );
  int const index = m_file.m_queue.top().m_index;

  m_file.m_queue.pop();

  if (!m_file.PushNextValue(index))
    m_end = m_file.m_queue.empty();
}

string StringsFile::FormatFilePath(int i) const
{
  return m_filePath + string(".") + strings::to_string(i);
}

void StringsFile::Flush()
{
  sort(m_strings.begin(), m_strings.end());

  FileWriter w(FormatFilePath(m_index++));
  for_each(m_strings.begin(), m_strings.end(), bind(&StringT::Write<FileWriter>, _1, ref(w)));

  m_strings.clear();
}

bool StringsFile::PushNextValue(int i)
{
  try
  {
    StringT s;
    s.Read(m_readers[i]);

    m_queue.push(QValue(s, i));
    return true;
  }
  catch (SourceOutOfBoundsException const &)
  {
    return false;
  }
}

void StringsFile::EndAdding()
{
  Flush();
}

void StringsFile::OpenForRead()
{
  for (int i = 0; i < m_index; ++i)
  {
    m_readers.push_back(ReaderT(new FileReader(FormatFilePath(i), 6, 1)));

    CHECK ( PushNextValue(i), () );
  }
}
