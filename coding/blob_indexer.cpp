#include "coding/blob_indexer.hpp"

/*
#include "coding/writer.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"


BlobIndexer::BlobIndexer(Writer & writer,
                         size_t maxUncompressedChunkSize,
                         CompressorType const & compressor) :
  m_writer(writer),
  m_maxUncompressedChunkSize(min(int(maxUncompressedChunkSize), (1 << BITS_IN_CHUNK_SIZE) - 1)),
  m_compressor(compressor),
  m_totalBlobSizeUncompressed(0),
  m_maxBlobSize(0),
  m_largeBlobCount(0)
{
  ASSERT_LESS(maxUncompressedChunkSize, (1 << BITS_IN_CHUNK_SIZE), ());
  CHECK_EQUAL(m_writer.Pos(), 0, ("Writer should not have something written already"));

  // Write header.
  char const header[] = "Blb";
  m_writer.Write(header, 3);
  WriteToSink(m_writer, static_cast<uint8_t>(BITS_IN_CHUNK_SIZE));
}

uint64_t BlobIndexer::AddBlob(string const & blob)
{
  if (blob.size() > m_maxUncompressedChunkSize)
  {
    LOG(LINFO, ("Blob bigger than chunk:", m_blobChunkAndOffset.size(), blob.size(),
                blob.substr(0, 64)));
    ++m_largeBlobCount;
  }

  if (m_currentChunk.size() + blob.size() > m_maxUncompressedChunkSize)
    FlushChunk();

  m_blobChunkAndOffset.push_back(
        (m_chunkOffset.size() << BITS_IN_CHUNK_SIZE) + m_currentChunk.size());

  m_currentChunk.insert(m_currentChunk.end(), blob.begin(), blob.end());

  return m_blobChunkAndOffset.size() - 1;
}

void BlobIndexer::FlushChunk()
{
  if (!m_currentChunk.empty())
  {
    string compressedChunk;
    m_compressor(&m_currentChunk[0], m_currentChunk.size(), compressedChunk);
    m_writer.Write(compressedChunk.data(), compressedChunk.size());
    WriteToSink(m_writer, static_cast<uint32_t>(m_currentChunk.size()));
    uint32_t const chunkPrevOffset = (m_chunkOffset.empty() ? 0 : m_chunkOffset.back());
    m_chunkOffset.push_back(compressedChunk.size() + 4 + chunkPrevOffset);
    m_currentChunk.clear();
  }
}

BlobIndexer::~BlobIndexer()
{
  FlushChunk();

  for (size_t i = 0; i < m_chunkOffset.size(); ++i)
    WriteToSink(m_writer, m_chunkOffset[i]);
  for (size_t i = 0; i < m_blobChunkAndOffset.size(); ++i)
    WriteToSink(m_writer, m_blobChunkAndOffset[i]);
  WriteToSink(m_writer, static_cast<uint32_t>(m_blobChunkAndOffset.size()));
}
*/
