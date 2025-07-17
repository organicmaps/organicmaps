#pragma once

#include "coding/bwt.hpp"
#include "coding/huffman.hpp"
#include "coding/move_to_front.hpp"
#include "coding/varint.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

namespace coding
{
class BWTCoder
{
public:
  using BufferT = std::vector<uint8_t>;

  struct Params
  {
    size_t m_blockSize = 32000;
  };

  template <typename Sink>
  static void EncodeAndWriteBlock(Sink & sink, size_t n, uint8_t const * s, BufferT & bwtBuffer)
  {
    bwtBuffer.resize(n);
    auto const start = BWT(n, s, bwtBuffer.data());

    MoveToFront mtf;
    for (auto & b : bwtBuffer)
      b = mtf.Transform(b);

    WriteVarUint(sink, start);

    HuffmanCoder huffman;
    huffman.Init(bwtBuffer.begin(), bwtBuffer.end());
    huffman.WriteEncoding(sink);
    huffman.EncodeAndWrite(sink, bwtBuffer.begin(), bwtBuffer.end());
  }

  template <typename Sink>
  static void EncodeAndWriteBlock(Sink & sink, size_t n, uint8_t const * s)
  {
    BufferT bwtBuffer;
    EncodeAndWriteBlock(sink, n, s, bwtBuffer);
  }

  template <typename Sink>
  static void EncodeAndWriteBlock(Sink & sink, std::string const & s)
  {
    EncodeAndWriteBlock(sink, s.size(), reinterpret_cast<uint8_t const *>(s.data()));
  }

  template <typename Sink>
  static void EncodeAndWrite(Params const & params, Sink & sink, size_t n, uint8_t const * s)
  {
    CHECK(params.m_blockSize != 0, ());
    CHECK_GREATER(n + params.m_blockSize, n, ());

    BufferT bwtBuffer;

    size_t const numBlocks = (n + params.m_blockSize - 1) / params.m_blockSize;
    WriteVarUint(sink, numBlocks);
    for (size_t i = 0; i < n; i += params.m_blockSize)
    {
      auto const m = std::min(n - i, params.m_blockSize);
      EncodeAndWriteBlock(sink, m, s + i, bwtBuffer);
    }
  }

  template <typename Source>
  static void ReadAndDecodeBlock(Source & source, BufferT & bwtBuffer, BufferT & revBuffer)
  {
    auto const start = ReadVarUint<uint64_t, Source>(source);

    HuffmanCoder huffman;
    huffman.ReadEncoding(source);

    bwtBuffer.clear();
    huffman.ReadAndDecode(source, std::back_inserter(bwtBuffer));

    size_t const n = bwtBuffer.size();
    MoveToFront mtf;
    for (size_t i = 0; i < n; ++i)
    {
      auto const b = mtf[bwtBuffer[i]];
      bwtBuffer[i] = b;
      mtf.Transform(b);
    }

    if (n != 0)
      CHECK_LESS(start, n, ());

    revBuffer.resize(n);
    RevBWT(n, static_cast<size_t>(start), bwtBuffer.data(), revBuffer.data());
  }

  template <typename Source>
  static BufferT ReadAndDecodeBlock(Source & source)
  {
    BufferT bwtBuffer, revBuffer;
    ReadAndDecodeBlock(source, bwtBuffer, revBuffer);
    return revBuffer;
  }

  template <typename Source, typename OutIt>
  static OutIt ReadAndDecode(Source & source, OutIt it)
  {
    auto const numBlocks = ReadVarUint<uint64_t, Source>(source);
    CHECK_LESS(numBlocks, std::numeric_limits<size_t>::max(), ());

    BufferT bwtBuffer, revBuffer;

    for (size_t i = 0; i < static_cast<size_t>(numBlocks); ++i)
    {
      ReadAndDecodeBlock(source, bwtBuffer, revBuffer);
      std::copy(revBuffer.begin(), revBuffer.end(), it);
    }
    return it;
  }
};
}  // namespace coding
