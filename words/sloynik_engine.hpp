#pragma once
#include "common.hpp"
#include "../base/base.hpp"
#include "../std/function.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/string.hpp"
#include "../std/utility.hpp"

class FileReader;

namespace sl
{

class Dictionary;
class SortedIndex;

class SloynikEngine
{
public:
  SloynikEngine(string const & dictionary,
                string const & index,
                sl::StrFn const & strFn);

  ~SloynikEngine();

  typedef uint32_t WordId;

  struct WordInfo
  {
    string m_Word;
  };

  void GetWordInfo(WordId word, WordInfo & res) const;

  struct SearchResult
  {
    SearchResult() : m_FirstMatched(0) {}
    WordId m_FirstMatched;
  };

  void Search(string const & prefix, SearchResult & res) const;

  uint32_t WordCount() const;

  struct ArticleData
  {
    string m_HTML;

    void swap(ArticleData & o)
    {
      m_HTML.swap(o.m_HTML);
    }
  };

  void GetArticleData(WordId word, ArticleData & data) const;

private:
  scoped_ptr<sl::Dictionary> m_pDictionary;
  //scoped_ptr<FileReader> m_pIndexReader;
  scoped_ptr<sl::SortedIndex> m_pSortedIndex;
};

}
