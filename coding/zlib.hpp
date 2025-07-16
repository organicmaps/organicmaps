#pragma once

#include "base/assert.hpp"
#include "base/macros.hpp"

#include <algorithm>
#include <cstddef>
#include <string>

#include "zlib.h"

namespace coding
{
// Following classes are wrappers around ZLib routines.
//
// *NOTE* All Inflate() and Deflate() methods may return false in case
// of errors. In this case the output sequence may be already
// partially formed, so the user needs to implement their own
// roll-back strategy.
class ZLib
{
public:
  class Inflate
  {
  public:
    enum class Format
    {
      ZLib,
      GZip,
      Both
    };

    explicit Inflate(Format format) noexcept : m_format(format) {}

    template <typename OutIt>
    bool operator()(void const * data, size_t size, OutIt out) const
    {
      if (data == nullptr)
        return false;
      InflateProcessor processor(m_format, data, size);
      return Process(processor, out);
    }

    template <typename OutIt>
    bool operator()(std::string const & s, OutIt out) const
    {
      return (*this)(s.c_str(), s.size(), out);
    }

  private:
    Format const m_format;
  };

  class Deflate
  {
  public:
    enum class Format
    {
      ZLib,
      GZip
    };

    enum class Level
    {
      NoCompression,
      BestSpeed,
      BestCompression,
      DefaultCompression
    };

    Deflate(Format format, Level level) noexcept : m_format(format), m_level(level) {}

    template <typename OutIt>
    bool operator()(void const * data, size_t size, OutIt out) const
    {
      if (data == nullptr)
        return false;
      DeflateProcessor processor(m_format, m_level, data, size);
      return Process(processor, out);
    }

    template <typename OutIt>
    bool operator()(std::string const & s, OutIt out) const
    {
      return (*this)(s.c_str(), s.size(), out);
    }

  private:
    Format const m_format;
    Level const m_level;
  };

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
      std::copy(m_buffer, m_buffer + kBufferSize - m_stream.avail_out, out);
      m_stream.next_out = m_buffer;
      m_stream.avail_out = kBufferSize;
    }

  protected:
    z_stream m_stream;
    bool m_init = false;
    unsigned char m_buffer[kBufferSize] = {};

    DISALLOW_COPY_AND_MOVE(Processor);
  };

  class DeflateProcessor final : public Processor
  {
  public:
    DeflateProcessor(Deflate::Format format, Deflate::Level level, void const * data, size_t size) noexcept;
    virtual ~DeflateProcessor() noexcept override;

    int Process(int flush);

    DISALLOW_COPY_AND_MOVE(DeflateProcessor);
  };

  class InflateProcessor final : public Processor
  {
  public:
    InflateProcessor(Inflate::Format format, void const * data, size_t size) noexcept;
    virtual ~InflateProcessor() noexcept override;

    int Process(int flush);

    DISALLOW_COPY_AND_MOVE(InflateProcessor);
  };

  template <typename Processor, typename OutIt>
  static bool Process(Processor & processor, OutIt out)
  {
    if (!processor.IsInit())
      return false;

    int ret = Z_OK;
    while (true)
    {
      int const flush = (processor.ConsumedAll() || ret == Z_STREAM_END) ? Z_FINISH : Z_NO_FLUSH;

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
    return processor.ConsumedAll();
  }
};
}  // namespace coding
