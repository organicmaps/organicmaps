#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

#include <vector>

FileWriter::FileWriter(std::string const & fileName, FileWriter::Op op)
  : m_pFileData(std::make_unique<base::FileData>(fileName, static_cast<base::FileData::Op>(op)))
{}

FileWriter::~FileWriter() noexcept(false)
{
  // Note: FileWriter::Flush will be called (like non virtual method).
  Flush();
}

uint64_t FileWriter::Pos() const
{
  return m_pFileData->Pos();
}

void FileWriter::Seek(uint64_t pos)
{
  m_pFileData->Seek(pos);
}

void FileWriter::Write(void const * p, size_t size)
{
  m_pFileData->Write(p, size);
}

std::string const & FileWriter::GetName() const
{
  return m_pFileData->GetName();
}

uint64_t FileWriter::Size() const
{
  return m_pFileData->Size();
}

void FileWriter::Flush() noexcept(false)
{
  m_pFileData->Flush();
}

void FileWriter::DeleteFileX(std::string const & fName)
{
  UNUSED_VALUE(base::DeleteFileX(fName));
}
