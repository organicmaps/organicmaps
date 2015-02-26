#include "string_file.hpp"

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
#ifdef OMIM_OS_DESKTOP
  size_t const maxSize = 1000000;
#else
  size_t const maxSize = 30000;
#endif

  if (m_strings.size() >= maxSize)
    Flush();

  m_strings.push_back(s);
}

bool StringsFile::IteratorT::IsEnd() const
{
  return m_file.m_queue.empty();
}

StringsFile::StringT StringsFile::IteratorT::dereference() const
{
  ASSERT ( IsValid(), () );
  return m_file.m_queue.top().m_string;
}

void StringsFile::IteratorT::increment()
{
  ASSERT ( IsValid(), () );
  int const index = m_file.m_queue.top().m_index;

  m_file.m_queue.pop();

  if (!m_file.PushNextValue(index))
    m_end = IsEnd();
}

StringsFile::StringsFile(string const & fPath)
  : m_workerThread(1 /* maxTasks */)
{
  m_writer.reset(new FileWriter(fPath));
}

void StringsFile::Flush()
{
  shared_ptr<SortAndDumpStringsTask> task(
      new SortAndDumpStringsTask(*m_writer, m_offsets, m_strings));
  m_workerThread.Push(task);
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

  m_workerThread.RunUntilIdleAndStop();

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
