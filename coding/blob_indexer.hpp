#pragma once
#include "../std/function.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"
#include "../base/base.hpp"

class Writer;

class BlobIndexer
{
public:
  BlobIndexer(Writer & writer,
              size_t maxUncompressedChunkSize,
              function<void (char const *, size_t, string &)> const & compressor);
  ~BlobIndexer();

  // Add blob and return its id.
  uint64_t AddBlob(string const & blob);

  void LogStats() const;

private:
  void FlushChunk();

  Writer & m_writer;
  size_t const m_maxUncompressedChunkSize;
  function<void (char const *, size_t, string &)> const m_compressor;

  static uint32_t const BITS_IN_CHUNK_SIZE = 20;

  vector<uint32_t> m_chunkOffset;
  vector<uint32_t> m_blobChunkAndOffset;
  vector<char> m_currentChunk;

  // Just for stats.
  uint64_t m_totalBlobSizeUncompressed;
  uint32_t m_maxBlobSize;
  uint32_t m_largeBlobCount;
};
