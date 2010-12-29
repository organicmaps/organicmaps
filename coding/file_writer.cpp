#include "file_writer.hpp"
#include "internal/file_data.hpp"

#include "../std/cstdlib.hpp"
#include "../std/string.hpp"

#include "../../base/start_mem_debug.hpp"


FileWriter::FileWriter(FileWriter & rhs)
{
  m_pFileData.swap(rhs.m_pFileData);
}

FileWriter::FileWriter(string const & fileName, FileWriter::Op op)
  : m_pFileData(new FileData(fileName, static_cast<FileData::Op>(op)))
{
}

FileWriter::~FileWriter()
{
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

void FileWriter::DeleteFile(string const & fileName)
{
#ifdef OMIM_OS_BADA
  Osp::Io::File::Remove(fileName.c_str());
#else

  // Erase file.
  if (0 != remove(fileName.c_str()))
  {
    // additional check if file really was removed correctly
    FILE * f = fopen(fileName.c_str(), "r");
    if (f)
    {
      fclose(f);
      LOG(LERROR, ("File exists but can't be deleted. Sharing violation?", fileName));
    }
  }
#endif
}

uint64_t FileWriter::Size() const
{
  return m_pFileData->Size();
}

void FileWriter::Flush()
{
  m_pFileData->Flush();
}
