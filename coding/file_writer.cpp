#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"

FileWriter::FileWriter(FileWriter const & rhs)
: Writer(*this), m_bTruncOnClose(rhs.m_bTruncOnClose)
{
  m_pFileData.swap(const_cast<FileWriter &>(rhs).m_pFileData);
}

FileWriter::FileWriter(string const & fileName, FileWriter::Op op, bool bTruncOnClose)
: m_pFileData(new fdata_t(fileName, static_cast<fdata_t::Op>(op))), m_bTruncOnClose(bTruncOnClose)
{
}

FileWriter::~FileWriter()
{
  if (m_pFileData)
  {
    Flush();

    if (m_bTruncOnClose)
      m_pFileData->Truncate(Pos());
  }
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

void FileWriter::WritePaddingByEnd(size_t factor) { WritePadding(Size(), factor); }

void FileWriter::WritePaddingByPos(size_t factor) { WritePadding(Pos(), factor); }

string const & FileWriter::GetName() const
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

void FileWriter::Reserve(uint64_t size)
{
  if (size > 0)
  {
    m_pFileData->Seek(size-1);
    uint8_t b = 0;
    m_pFileData->Write(&b, 1);
  }
}

void FileWriter::DeleteFileX(string const & fName)
{
  (void)my::DeleteFileX(fName);
}

void FileWriter::WritePadding(uint64_t offset, uint64_t factor)
{
  ASSERT(factor > 1, ());
  uint64_t const padding = ((offset + factor - 1) / factor) * factor - offset;
  if (!padding)
    return;
  vector<uint8_t> buffer(padding);
  Write(buffer.data(), buffer.size());
}
