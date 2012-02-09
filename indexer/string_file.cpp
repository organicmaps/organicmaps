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
  rw::WriteVectorOfPOD(writer, m_val);

  return pos;
}

template <class TReader>
void StringsFile::StringT::Read(TReader & src)
{
  rw::Read(src, m_name);
  rw::ReadVectorOfPOD(src, m_val);
}

bool StringsFile::StringT::operator < (StringT const & name) const
{
  if (m_name != name.m_name)
    return (m_name < name.m_name);

  return (m_val < name.m_val);
}

bool StringsFile::StringT::operator == (StringT const & name) const
{
  return (m_name == name.m_name && m_val == name.m_val);
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

StringsFile::StringsFile(string const & fPath)
{
  m_writer.reset(new FileWriter(fPath));
}

void StringsFile::Flush()
{
  // store starting offset
  uint64_t const pos = m_writer->Pos();
  m_offsets.push_back(make_pair(pos, pos));

  // sort strings
  sort(m_strings.begin(), m_strings.end());

  // write strings to file
  for_each(m_strings.begin(), m_strings.end(), bind(&StringT::Write<FileWriter>, _1, ref(*m_writer)));

  // store end offset
  m_offsets.back().second = m_writer->Pos();

  m_strings.clear();
}

bool StringsFile::PushNextValue(size_t i)
{
  // reach the end of the portion file
  if (m_offsets[i].first >= m_offsets[i].second)
    return false;

  // init source to needed offset
  ReaderSource<FileReader> src(*m_reader);
  src.Skip(m_offsets[i].first);

  // read string
  StringT s;
  s.Read(src);

  // update offset
  m_offsets[i].first = src.Pos();

  // push value to queue
  m_queue.push(QValue(s, i));
  return true;
}

void StringsFile::EndAdding()
{
  Flush();

  m_writer->Flush();
}

void StringsFile::OpenForRead()
{
  string const fPath = m_writer->GetName();
  m_writer.reset();

  m_reader.reset(new FileReader(fPath));

  for (size_t i = 0; i < m_offsets.size(); ++i)
    PushNextValue(i);
}
