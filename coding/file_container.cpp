#include "../base/SRC_FIRST.hpp"

#include "file_container.hpp"
#include "varint.hpp"


FilesContainerR::FilesContainerR(string const & fName)
: m_source(fName)
{
  ReaderSource<FileReader> src(m_source);

  uint64_t const offset = ReadVarUint<uint64_t>(src);
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

FileReader FilesContainerR::GetReader(Tag const & tag)
{
  info_cont_t::const_iterator i =
    lower_bound(m_info.begin(), m_info.end(), tag, less_info());

  if (i != m_info.end() && i->m_tag == tag)
    return m_source.SubReader(i->m_offset, i->m_size);
  else
    MYTHROW(Reader::OpenException, (tag));
}

FilesContainerW::FilesContainerW(string const & fName)
: m_name(fName)
{
  FileWriter writer(fName);
  uint64_t skip = 0;
  writer.Write(&skip, sizeof(skip));
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
  uint64_t const curr = SaveCurrentSize();

  m_info.push_back(Info(tag, curr));

  return FileWriter(m_name, FileWriter::OP_APPEND);
}

void FilesContainerW::Finish()
{
  uint64_t const curr = SaveCurrentSize();

  {
    FileWriter writer(m_name, FileWriter::OP_WRITE_EXISTING);
    writer.Write(&curr, sizeof(curr));
  }

  FileWriter writer(m_name, FileWriter::OP_APPEND);
  writer.Write(&curr, sizeof(curr));

  sort(m_info.begin(), m_info.end(), less_info());

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
