#include "../base/SRC_FIRST.hpp"

#include "file_container.hpp"
#include "varint.hpp"
#include "write_to_sink.hpp"


/////////////////////////////////////////////////////////////////////////////
// FilesContainerBase
/////////////////////////////////////////////////////////////////////////////

void FilesContainerBase::ReadInfo(FileReader & reader)
{
  uint64_t offset = ReadPrimitiveFromPos<uint64_t>(reader, 0);

  ReaderSource<FileReader> src(reader);
  src.Skip(offset);

  uint32_t const count = ReadVarUint<uint32_t>(src);
  m_info.resize(count);

  for (uint32_t i = 0; i < count; ++i)
  {
    uint32_t const tagSize = ReadVarUint<uint32_t>(src);
    m_info[i].m_tag.resize(tagSize);
    src.Read(&m_info[i].m_tag[0], tagSize);

    m_info[i].m_offset = ReadVarUint<uint64_t>(src);
    m_info[i].m_size = ReadVarUint<uint64_t>(src);
  }
}

/////////////////////////////////////////////////////////////////////////////
// FilesContainerR
/////////////////////////////////////////////////////////////////////////////

FilesContainerR::FilesContainerR(string const & fName,
                                 uint32_t logPageSize,
                                 uint32_t logPageCount)
: m_source(fName, logPageSize, logPageCount)
{
  ReadInfo(m_source);
}

FileReader FilesContainerR::GetReader(Tag const & tag) const
{
  info_cont_t::const_iterator i =
    lower_bound(m_info.begin(), m_info.end(), tag, less_info());

  if (i != m_info.end() && i->m_tag == tag)
    return m_source.SubReader(i->m_offset, i->m_size);
  else
    MYTHROW(Reader::OpenException, (tag));
}

/////////////////////////////////////////////////////////////////////////////
// FilesContainerW
/////////////////////////////////////////////////////////////////////////////

FilesContainerW::FilesContainerW(string const & fName, FileWriter::Op op)
: m_name(fName)
{
  if (op == FileWriter::OP_APPEND)
  {
    FileReader reader(fName);
    ReadInfo(reader);
    m_needRewrite = true;
  }

  if (m_info.empty())
  {
    FileWriter writer(fName);
    uint64_t skip = 0;
    writer.Write(&skip, sizeof(skip));
    m_needRewrite = false;
  }
}

uint64_t FilesContainerW::SaveCurrentSize()
{
  uint64_t const curr = FileReader(m_name).Size();
  if (!m_info.empty())
    m_info.back().m_size = curr - m_info.back().m_offset;
  return curr;
}

FileWriter FilesContainerW::GetWriter(Tag const & tag)
{
  if (m_needRewrite)
  {
    m_needRewrite = false;
    ASSERT ( !m_info.empty(), () );

    uint64_t const curr = m_info.back().m_offset + m_info.back().m_size;
    m_info.push_back(Info(tag, curr));

    FileWriter writer(m_name, FileWriter::OP_WRITE_EXISTING);
    writer.Seek(curr);
    return writer;
  }
  else
  {
    uint64_t const curr = SaveCurrentSize();
    m_info.push_back(Info(tag, curr));
    return FileWriter(m_name, FileWriter::OP_APPEND);
  }
}

void FilesContainerW::Append(string const & fName, Tag const & tag)
{
  uint64_t const bufferSize = 4*1024;
  char buffer[bufferSize];

  FileReader reader(fName);
  ReaderSource<FileReader> src(reader);
  FileWriter writer = GetWriter(tag);

  uint64_t size = reader.Size();
  while (size > 0)
  {
    size_t const curr = static_cast<size_t>(min(bufferSize, size));

    src.Read(&buffer[0], curr);
    writer.Write(&buffer[0], curr);

    size -= curr;
  }
}

void FilesContainerW::Append(vector<char> const & buffer, Tag const & tag)
{
  if (!buffer.empty())
    GetWriter(tag).Write(&buffer[0], buffer.size());
}

void FilesContainerW::Finish()
{
  {
    uint64_t const curr = SaveCurrentSize();
    FileWriter writer(m_name, FileWriter::OP_WRITE_EXISTING);
    writer.Seek(0);
    WriteToSink(writer, curr);
  }

  sort(m_info.begin(), m_info.end(), less_info());

  FileWriter writer(m_name, FileWriter::OP_APPEND);

  uint32_t const count = m_info.size();
  WriteVarUint(writer, count);

  for (uint32_t i = 0; i < count; ++i)
  {
    size_t const tagSize = m_info[i].m_tag.size();
    WriteVarUint(writer, tagSize);
    writer.Write(&m_info[i].m_tag[0], tagSize);

    WriteVarUint(writer, m_info[i].m_offset);
    WriteVarUint(writer, m_info[i].m_size);
  }
}
