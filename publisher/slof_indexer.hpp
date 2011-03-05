#pragma once
#include "../base/base.hpp"
#include "../std/function.hpp"
#include "../std/set.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"
#include "../std/vector.hpp"

class Writer;

namespace sl
{

class SlofIndexer
{
public:
  SlofIndexer(Writer & writer,
            size_t maxUncompressedArticleChunkSize,
            function<void (char const *, size_t, string &)> const & compressor);
  ~SlofIndexer();

  // Add article and return its id.
  uint64_t AddArticle(string const & article, bool forceChunkFlush = false);

  // Add key with given article id. Keys may be passed in arbitry order.
  void AddKey(string const & word, uint64_t articleId);

  void LogStats() const;

private:
  void FlushArticleChunk();

  Writer & m_Writer;
  size_t const m_MaxUncompressedArticleChunkSize;
  function<void (char const *, size_t, string &)> m_Compressor;
  typedef set<pair<string, uint64_t> > WordsContainerType;
  WordsContainerType m_Words;
  uint64_t const m_ArticleOffset;
  string m_CurrentArticleChunk;
  vector<uint32_t> m_ArticleSizesInChunk;
  uint32_t m_ArticleCount;

  // Just for stats.
  uint32_t m_ArticleChunkCount;
  uint64_t m_TotalArticleSizeUncompressed;
  uint32_t m_MaxArticleSize;
};

}
