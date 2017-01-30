#pragma once

#include "base/assert.hpp"
#include "base/macros.hpp"

#include "std/algorithm.hpp"
#include "std/string.hpp"

#include "zlib.h"

namespace coding
{
// Following class is a wrapper around ZLib routines.
//
// *NOTE* All Inflate() and Deflate() methods may return false in case
// of errors. In this case the output sequence may be already
// partially formed, so the user needs to implement their own
// roll-back strategy.
class ZLib
{
public:
  enum class Level
  {
    NoCompression,
    BestSpeed,
    BestCompression,
    DefaultCompression
  };

  template <typename OutIt>
  static bool Deflate(void const * data, size_t size, Level level, OutIt out)
  {
    if (data == nullptr)
      return false;
    DeflateProcessor processor(data, size, level);
    return Process(processor, out);
  }

  template <typename OutIt>
  static bool Deflate(string const & s, Level level, OutIt out)
  {
    return Deflate(s.c_str(), s.size(), level, out);
  }

  template <typename OutIt>
  static bool Inflate(void const * data, size_t size, OutIt out)
  {
    if (data == nullptr)
      return false;
    InflateProcessor processor(data, size);
    return Process(processor, out);
  }

  template <typename OutIt>
  static bool Inflate(string const & s, OutIt out)
  {
    return Inflate(s.c_str(), s.size(), out);
  }

private:
  class Processor
  {
  public:
    static size_t constexpr kBufferSize = 1024;

    Processor(void const * data, size_t size) noexcept;
    virtual ~Processor() noexcept = default;

    inline bool IsInit() const noexcept { return m_init; }
    bool ConsumedAll() const;
    bool BufferIsFull() const;

    template <typename OutIt>
    void MoveOut(OutIt out)
    {
      ASSERT(IsInit(), ());
      copy(m_buffer, m_buffer + kBufferSize - m_stream.avail_out, out);
      m_stream.next_out = m_buffer;
      m_stream.avail_out = kBufferSize;
    }

  protected:
    z_stream m_stream;
    bool m_init;
    unsigned char m_buffer[kBufferSize];

    DISALLOW_COPY_AND_MOVE(Processor);
  };

  class DeflateProcessor final : public Processor
  {
  public:
    DeflateProcessor(void const * data, size_t size, Level level) noexcept;
    virtual ~DeflateProcessor() noexcept override;

    int Process(int flush);

    DISALLOW_COPY_AND_MOVE(DeflateProcessor);
  };

  class InflateProcessor final : public Processor
  {
  public:
    InflateProcessor(void const * data, size_t size) noexcept;
    virtual ~InflateProcessor() noexcept override;

    int Process(int flush);

    DISALLOW_COPY_AND_MOVE(InflateProcessor);
  };

  template <typename Processor, typename OutIt>
  static bool Process(Processor & processor, OutIt out)
  {
    if (!processor.IsInit())
      return false;

    while (true)
    {
      int const flush = processor.ConsumedAll() ? Z_FINISH : Z_NO_FLUSH;
      int ret = Z_OK;

      while (true)
      {
        ret = processor.Process(flush);
        if (ret != Z_OK && ret != Z_STREAM_END)
          return false;

        if (!processor.BufferIsFull())
          break;

        processor.MoveOut(out);
      }

      if (flush == Z_FINISH && ret == Z_STREAM_END)
        break;
    }

    processor.MoveOut(out);
    return true;
  }
};
}  // namespace coding
