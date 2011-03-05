#include "slof_indexer.hpp"
#include "../words/slof.hpp"
#include "../coding/byte_stream.hpp"
#include "../coding/endianness.hpp"
#include "../coding/varint.hpp"
#include "../coding/writer.hpp"
#include "../coding/write_to_sink.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/logging.hpp"
#include "../std/algorithm.hpp"
#include "../std/set.hpp"
#include "../std/string.hpp"

namespace
{
  template <typename T> uint8_t VarUintSize(T x)
  {
    uint8_t res = 0;
    while (x > 127)
    {
      ++res;
      x >>= 7;
    }
    return res + 1;
  }
}

sl::SlofIndexer::SlofIndexer(Writer & writer,
                             size_t maxUncompressedArticleChunkSize,
                             function<void (char const *, size_t, string &)> const & compressor) :
m_Writer(writer),
m_MaxUncompressedArticleChunkSize(maxUncompressedArticleChunkSize),
m_Compressor(compressor),
m_ArticleOffset(m_Writer.Pos() + sizeof(sl::SlofHeader)),
m_ArticleCount(0),
m_ArticleChunkCount(0),
m_MaxArticleSize(0)
{
  CHECK_LESS(maxUncompressedArticleChunkSize, 1 << 24, ());
  m_Writer.Seek(sizeof(sl::SlofHeader));
  CHECK_EQUAL(m_ArticleOffset, m_Writer.Pos(), ());
}

void sl::SlofIndexer::AddKey(string const & word, uint64_t articleId)
{
  CHECK(!word.empty(), ());
  WordsContainerType::const_iterator it = m_Words.lower_bound(make_pair(word, 0ULL));
  if (it != m_Words.end() && it->first == word)
  {
    LOG(LINFO, ("Duplicate key:", word, it->second, articleId));
  }
  CHECK(m_Words.insert(make_pair(word, articleId)).second, (word, articleId));
}

uint64_t sl::SlofIndexer::AddArticle(string const & article, bool forceChunkFlush)
{
  // if (article.size() > m_MaxUncompressedArticleChunkSize)
  //   LOG(LWARNING, ("Article bigger than chunk:", article.size(), article.substr(0, 64)));

  if (m_CurrentArticleChunk.size() + article.size() > m_MaxUncompressedArticleChunkSize ||
      forceChunkFlush)
    FlushArticleChunk();

  uint64_t const articleId =
      ((m_Writer.Pos() - m_ArticleOffset) << 24) + m_ArticleSizesInChunk.size();
  m_CurrentArticleChunk += article;
  m_ArticleSizesInChunk.push_back(article.size());

  ++m_ArticleCount;
  m_TotalArticleSizeUncompressed += article.size();
  m_MaxArticleSize = max(m_MaxArticleSize, static_cast<uint32_t>(article.size()));

  return articleId;
}

void sl::SlofIndexer::FlushArticleChunk()
{
  if (m_ArticleSizesInChunk.empty())
    return;

  vector<char> chunkHeader;
  { // Write chunk header.
    {
      PushBackByteSink<vector<char> > sink(chunkHeader);
      // Write decompressed size of all articles.
      WriteVarUint(sink, m_CurrentArticleChunk.size());
      // Write individual article sizes.
      for (size_t i = 0; i < m_ArticleSizesInChunk.size(); ++i)
        WriteVarUint(sink, m_ArticleSizesInChunk[i]);
    }
    { // Write size of the header at the beginning of the header.
      vector<char> chunkHeaderSize;
      PushBackByteSink<vector<char> > sink(chunkHeaderSize);
      WriteVarUint(sink, chunkHeader.size());
      chunkHeader.insert(chunkHeader.begin(), chunkHeaderSize.begin(), chunkHeaderSize.end());
    }
  }

  // Compress the article chunk.
  string compressedArticleChunk;
  m_Compressor(&m_CurrentArticleChunk[0], m_CurrentArticleChunk.size(), compressedArticleChunk);

  // Write everything.
  WriteToSink(m_Writer, static_cast<uint32_t>(chunkHeader.size() + compressedArticleChunk.size()));
  m_Writer.Write(&chunkHeader[0], chunkHeader.size());
  m_Writer.Write(&compressedArticleChunk[0], compressedArticleChunk.size());

  // Reset everything.
  m_CurrentArticleChunk.clear();
  m_ArticleSizesInChunk.clear();
  ++m_ArticleChunkCount;
}

void sl::SlofIndexer::LogStats() const
{
  LOG(LINFO, ("Dictionary stats"));
  set<uint64_t> articleIds;
  uint32_t maxKeyLength = 0, totalWordLength = 0, dupKeysCount = 0;
  for (WordsContainerType::const_iterator it = m_Words.begin(); it != m_Words.end(); ++it)
  {
    WordsContainerType::const_iterator next = it;
    ++next;
    if (next != m_Words.end() && next->first == it->first)
      ++dupKeysCount;
    maxKeyLength = max(maxKeyLength, static_cast<uint32_t>(it->first.size()));
    totalWordLength += it->first.size();
    articleIds.insert(it->second);
  }

  CHECK_EQUAL(m_ArticleCount, articleIds.size(), ());

  LOG(LINFO, ("Keys:", m_Words.size()));
  LOG(LINFO, ("Unique keys:", m_Words.size() - dupKeysCount));
  LOG(LINFO, ("Duplicate keys:", dupKeysCount));
  LOG(LINFO, ("Duplicate keys %:", 100.0 * dupKeysCount / m_Words.size()));
  LOG(LINFO, ("Max key length:", maxKeyLength));
  LOG(LINFO, ("Average key length:", totalWordLength * 1.0 / m_Words.size()));
  LOG(LINFO, ("Articles:", m_ArticleCount));
  LOG(LINFO, ("Keys per article:", m_Words.size() * 1.0 / m_ArticleCount));
  LOG(LINFO, ("Article chunks:", m_ArticleChunkCount));
  LOG(LINFO, ("Articles per chunk:", m_ArticleCount * 1.0 / m_ArticleChunkCount));
  LOG(LINFO, ("Average article size:", m_TotalArticleSizeUncompressed * 1.0 / m_ArticleCount));
  LOG(LINFO, ("Max article size:", m_MaxArticleSize));
}

sl::SlofIndexer::~SlofIndexer()
{
  FlushArticleChunk();

  // Filling in header information.
  sl::SlofHeader header;
  memcpy(&header.m_Signature, "slof", 4);
  header.m_MajorVersion = SwapIfBigEndian(uint16_t(1));
  header.m_MinorVersion = SwapIfBigEndian(uint16_t(1));
  header.m_KeyCount = SwapIfBigEndian(static_cast<uint32_t>(m_Words.size()));
  header.m_ArticleCount = SwapIfBigEndian(m_ArticleCount);
  header.m_ArticleOffset = SwapIfBigEndian(static_cast<uint64_t>(sizeof(header)));

  // Writing key index.
  header.m_KeyIndexOffset = SwapIfBigEndian(m_Writer.Pos());
  {
    WriteToSink(m_Writer, static_cast<uint32_t>(0));
    uint32_t cumSize = 0;
    for (WordsContainerType::const_iterator it = m_Words.begin(); it != m_Words.end(); ++it)
    {
      cumSize += it->first.size();
      cumSize += VarUintSize(it->second >> 24);
      cumSize += VarUintSize(it->second & 0xFFFFF);
      WriteToSink(m_Writer, cumSize);
    }
  }

  // Writing key data.
  header.m_KeyDataOffset = SwapIfBigEndian(m_Writer.Pos());
  for (WordsContainerType::const_iterator it = m_Words.begin(); it != m_Words.end(); ++it)
  {
    WriteVarUint(m_Writer, it->second >> 24);
    WriteVarUint(m_Writer, it->second & 0xFFFFFF);
    m_Writer.Write(&it->first[0], it->first.size());
  }

  // Writing header.
  uint64_t const lastPos = m_Writer.Pos();
  m_Writer.Seek(0);
  m_Writer.Write(&header, sizeof(header));
  m_Writer.Seek(lastPos);
}
