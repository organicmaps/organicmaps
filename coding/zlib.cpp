#include "coding/zlib.hpp"

namespace coding
{
namespace
{
int LevelToInt(ZLib::Level level)
{
  switch (level)
  {
  case ZLib::Level::NoCompression: return Z_NO_COMPRESSION;
  case ZLib::Level::BestSpeed: return Z_BEST_SPEED;
  case ZLib::Level::BestCompression: return Z_BEST_COMPRESSION;
  case ZLib::Level::DefaultCompression: return Z_DEFAULT_COMPRESSION;
  }
}
}  // namespace

// ZLib::Processor ---------------------------------------------------------------------------------
ZLib::Processor::Processor(char const * data, size_t size) : m_init(false)
{
  m_stream.next_in = const_cast<unsigned char *>(reinterpret_cast<unsigned char const *>(data));
  m_stream.avail_in = size;

  m_stream.next_out = reinterpret_cast<unsigned char *>(m_buffer);
  m_stream.avail_out = kBufferSize;

  m_stream.zalloc = Z_NULL;
  m_stream.zfree = Z_NULL;
  m_stream.opaque = Z_NULL;
}

bool ZLib::Processor::ConsumedAll() const
{
  ASSERT(IsInit(), ());
  return m_stream.avail_in == 0;
}

bool ZLib::Processor::BufferIsFull() const
{
  ASSERT(IsInit(), ());
  return m_stream.avail_out == 0;
}

// ZLib::Deflate -----------------------------------------------------------------------------------
ZLib::DeflateProcessor::DeflateProcessor(char const * data, size_t size, ZLib::Level level)
  : Processor(data, size)
{
  int const ret = deflateInit(&m_stream, LevelToInt(level));
  m_init = (ret == Z_OK);
}

ZLib::DeflateProcessor::~DeflateProcessor()
{
  if (m_init)
    deflateEnd(&m_stream);
}

int ZLib::DeflateProcessor::Process(int flush)
{
  ASSERT(IsInit(), ());
  return deflate(&m_stream, flush);
}

// ZLib::Inflate -----------------------------------------------------------------------------------
ZLib::InflateProcessor::InflateProcessor(char const * data, size_t size) : Processor(data, size)
{
  int const ret = inflateInit(&m_stream);
  m_init = (ret == Z_OK);
}

ZLib::InflateProcessor::~InflateProcessor()
{
  if (m_init)
    inflateEnd(&m_stream);
}

int ZLib::InflateProcessor::Process(int flush)
{
  ASSERT(IsInit(), ());
  return inflate(&m_stream, flush);
}
}  // namespace coding
