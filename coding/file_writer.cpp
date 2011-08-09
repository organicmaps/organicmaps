#include "file_writer.hpp"
#include "internal/file_data.hpp"

#include "../../base/start_mem_debug.hpp"


FileWriter::FileWriter(FileWriter const & rhs) : Writer(*this)
{
  m_pFileData.swap(const_cast<FileWriter &>(rhs).m_pFileData);
}

FileWriter::FileWriter(string const & fileName, FileWriter::Op op)
  : m_pFileData(new fdata_t(fileName, static_cast<fdata_t::Op>(op)))
{
}

FileWriter::~FileWriter()
{
  if (m_pFileData)
    Flush();
}

int64_t FileWriter::Pos() const
{
  return m_pFileData->Pos();
}

void FileWriter::Seek(int64_t pos)
{
  ASSERT_GREATER_OR_EQUAL(pos, 0, ());
  m_pFileData->Seek(pos);
}

void FileWriter::Write(void const * p, size_t size)
{
  m_pFileData->Write(p, size);
}

string FileWriter::GetName() const
{
  return m_pFileData->GetName();
}

uint64_t FileWriter::Size() const
{
  return m_pFileData->Size();
}

void FileWriter::Flush()
{
  m_pFileData->Flush();
}

void FileWriter::DeleteFileX(string const & fName)
{
    my::DeleteFileX(fName);
}

