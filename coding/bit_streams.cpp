#include "bit_streams.hpp"

#include "reader.hpp"
#include "writer.hpp"

BitSink::BitSink(Writer & writer)
  : m_writer(writer), m_lastByte(0), m_size(0) {}
  
BitSink::~BitSink()
{
  if (m_size % 8 > 0) m_writer.Write(&m_lastByte, 1);
}

void BitSink::Write(uint64_t bits, uint32_t writeSize)
{
  if (writeSize == 0) return;
  CHECK_LESS_OR_EQUAL(writeSize, 64, ());
  m_totalBits += writeSize;
  uint32_t remSize = m_size % 8;
  if (writeSize > 64 - remSize)
  {
    uint64_t writeData = (bits << remSize) | m_lastByte;
    m_writer.Write(&writeData, sizeof(writeData));
    m_lastByte = uint8_t(bits >> (64 - remSize));
    m_size += writeSize;
  }
  else
  {
    if (remSize > 0)
    {
      bits <<= remSize;
      bits |= m_lastByte;
      writeSize += remSize;
      m_size -= remSize;
    }
    uint32_t writeBytesSize = writeSize / 8;
    m_writer.Write(&bits, writeBytesSize);
    m_lastByte = (bits >> (writeBytesSize * 8)) & ((1 << (writeSize % 8)) - 1);
    m_size += writeSize;
  }
}


BitSource::BitSource(Reader & reader)
  : m_reader(reader), m_serialCur(0), m_serialEnd(reader.Size()),
    m_bits(0), m_bitsSize(0), m_totalBitsRead(0) {}

uint64_t BitSource::Read(uint32_t readSize)
{
  uint32_t requestedReadSize = readSize;
  if (readSize == 0) return 0;
  CHECK_LESS_OR_EQUAL(readSize, 64, ());
  // First read, sets bits that are in the m_bits buffer.
  uint32_t firstReadSize = readSize <= m_bitsSize ? readSize : m_bitsSize;
  uint64_t result = m_bits & (~uint64_t(0) >> (64 - firstReadSize));
  m_bits >>= firstReadSize;
  m_bitsSize -= firstReadSize;
  readSize -= firstReadSize;
  // Second read, does an extra read using m_reader.
  if (readSize > 0)
  {
    size_t read_byte_size = m_serialCur + sizeof(m_bits) <= m_serialEnd ? sizeof(m_bits) : m_serialEnd - m_serialCur;
    m_reader.Read(m_serialCur, &m_bits, read_byte_size);
    m_serialCur += read_byte_size;
    m_bitsSize += read_byte_size * 8;
    if (readSize > m_bitsSize) CHECK_LESS_OR_EQUAL(readSize, m_bitsSize, ());
    result |= (m_bits & (~uint64_t(0) >> (64 - readSize))) << firstReadSize;
    m_bits >>= readSize;
    m_bitsSize -= readSize;
    readSize = 0;
  }
  m_totalBitsRead += requestedReadSize;
  return result;
}
