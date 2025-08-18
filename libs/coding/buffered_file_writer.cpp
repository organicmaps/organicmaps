#include "coding/buffered_file_writer.hpp"

#include "coding/internal/file_data.hpp"

#include "base/assert.hpp"

BufferedFileWriter::BufferedFileWriter(std::string const & fileName, Op operation /* = OP_WRITE_TRUNCATE */,
                                       size_t bufferSize /*  = 4096 */)
  : FileWriter(fileName, operation)
{
  CHECK_GREATER(bufferSize, 0, ());
  m_buf.reserve(bufferSize);
}

BufferedFileWriter::~BufferedFileWriter() noexcept(false)
{
  DropBuffer();
}

void BufferedFileWriter::Seek(uint64_t pos)
{
  DropBuffer();
  FileWriter::Seek(pos);
}

uint64_t BufferedFileWriter::Pos() const
{
  return FileWriter::Pos() + m_buf.size();
}

void BufferedFileWriter::Write(void const * p, size_t size)
{
  // Need to use pointer arithmetic.
  auto src = static_cast<uint8_t const *>(p);

  while (size >= m_buf.capacity() - m_buf.size())
  {
    if (m_buf.empty())
    {
      FileWriter::Write(src, m_buf.capacity());
      src += m_buf.capacity();
      size -= m_buf.capacity();
    }
    else
    {
      auto const copyCount = m_buf.capacity() - m_buf.size();
      std::copy(src, src + copyCount, std::back_inserter(m_buf));
      DropBuffer();
      src += copyCount;
      size -= copyCount;
    }
  }

  std::copy(src, src + size, std::back_inserter(m_buf));
}

uint64_t BufferedFileWriter::Size() const
{
  return FileWriter::Size() + m_buf.size();
}

void BufferedFileWriter::Flush()
{
  DropBuffer();
  FileWriter::Flush();
}

void BufferedFileWriter::DropBuffer()
{
  if (m_buf.empty())
    return;

  FileWriter::Write(m_buf.data(), m_buf.size());
  m_buf.clear();
}
