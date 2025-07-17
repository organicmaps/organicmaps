#include "coding/zlib.hpp"

#include "std/target_os.hpp"

namespace coding
{
namespace
{
int constexpr kGzipBits = 16;
int constexpr kBothBits = 32;

int ToInt(ZLib::Deflate::Level level)
{
  using Level = ZLib::Deflate::Level;
  switch (level)
  {
  case Level::NoCompression: return Z_NO_COMPRESSION;
  case Level::BestSpeed: return Z_BEST_SPEED;
  case Level::BestCompression: return Z_BEST_COMPRESSION;
  case Level::DefaultCompression: return Z_DEFAULT_COMPRESSION;
  }
  UNREACHABLE();
}
}  // namespace

// ZLib::Processor ---------------------------------------------------------------------------------
ZLib::Processor::Processor(void const * data, size_t size) noexcept : m_init(false)
{
  // next_in is defined as z_const (see
  // http://www.zlib.net/manual.html).  Sometimes it's a const (when
  // ZLIB_CONST is defined), sometimes not, it depends on the local
  // zconf.h. So, for portability, const_cast<...> is used here, but
  // in any case, zlib does not modify |data|.
  m_stream.next_in = static_cast<unsigned char *>(const_cast<void *>(data));
  m_stream.avail_in = static_cast<unsigned int>(size);

  m_stream.next_out = m_buffer;
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
ZLib::DeflateProcessor::DeflateProcessor(Deflate::Format format, Deflate::Level level,
                                         void const * data, size_t size) noexcept
  : Processor(data, size)
{
  auto bits = MAX_WBITS;
  switch (format)
  {
  case Deflate::Format::ZLib: break;
  case Deflate::Format::GZip: bits = bits | kGzipBits; break;
  }

  int const ret =
      deflateInit2(&m_stream, ToInt(level) /* level */, Z_DEFLATED /* method */,
                   bits /* windowBits */, 8 /* memLevel */, Z_DEFAULT_STRATEGY /* strategy */);
  m_init = (ret == Z_OK);
}

ZLib::DeflateProcessor::~DeflateProcessor() noexcept
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
ZLib::InflateProcessor::InflateProcessor(Inflate::Format format, void const * data,
                                         size_t size) noexcept
  : Processor(data, size)
{
  auto bits = MAX_WBITS;
  switch (format)
  {
  case Inflate::Format::ZLib: break;
  case Inflate::Format::GZip: bits = bits | kGzipBits; break;
  case Inflate::Format::Both: bits = bits | kBothBits; break;
  }
  int const ret = inflateInit2(&m_stream, bits);
  m_init = (ret == Z_OK);
}

ZLib::InflateProcessor::~InflateProcessor() noexcept
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
