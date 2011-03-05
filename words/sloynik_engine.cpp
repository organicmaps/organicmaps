#include "sloynik_engine.hpp"
#include "slof_dictionary.hpp"
#include "sloynik_index.hpp"
#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"
#include "../base/logging.hpp"
#include "../base/macros.hpp"

sl::SloynikEngine::SloynikEngine(string const & dictionaryPath,
                                 string const & indexPath,
                                 StrFn const & strFn)
{
  m_pDictionary.reset(new sl::SlofDictionary(new FileReader(dictionaryPath)));
  vector<uint64_t> stamp;
  stamp.push_back(strFn.m_PrimaryCompareId);
  stamp.push_back(strFn.m_SecondaryCompareId);
  string const stampPath = indexPath + ".stamp";
  bool needIndexBuild = false;
  try
  {
    FileReader stampReader(stampPath);
    if (stampReader.Size() != stamp.size() * sizeof(stamp[0]))
      needIndexBuild = true;
    else
    {
      vector<uint64_t> currentStamp(stamp.size());
      stampReader.Read(0, &currentStamp[0], stamp.size() * sizeof(stamp[0]));
      if (currentStamp != stamp)
        needIndexBuild = true;
    }
  }
  catch (RootException &)
  {
    needIndexBuild = true;
  }
  LOG(LINFO, ("Started sloynik engine. Words in the dictionary:", m_pDictionary->KeyCount()));
  // Uncomment to always rebuild the index: needIndexBuild = true;
  if (needIndexBuild)
  {
    FileWriter::DeleteFile(stampPath);
    sl::SortedIndex::Build(*m_pDictionary, strFn, indexPath);

    FileWriter stampWriter(stampPath);
    stampWriter.Write(&stamp[0], stamp.size() * sizeof(stamp[0]));
  }
  m_pIndexReader.reset(new FileReader(indexPath + ".idx"));
  m_pSortedIndex.reset(new sl::SortedIndex(*m_pDictionary, m_pIndexReader.get(), strFn));
}

sl::SloynikEngine::~SloynikEngine()
{
}

void sl::SloynikEngine::Search(string const & prefix, sl::SloynikEngine::SearchResult & res) const
{
  res.m_FirstMatched = m_pSortedIndex->PrefixSearch(prefix);
}

void sl::SloynikEngine::GetWordInfo(WordId word, sl::SloynikEngine::WordInfo & res) const
{
  m_pDictionary->KeyById(m_pSortedIndex->KeyIdByPos(word), res.m_Word);
}

uint32_t sl::SloynikEngine::WordCount() const
{
  return m_pDictionary->KeyCount();
}

void sl::SloynikEngine::GetArticleData(WordId wordId, ArticleData & data) const
{
  m_pDictionary->ArticleById(m_pSortedIndex->KeyIdByPos(wordId), data.m_HTML);
}
