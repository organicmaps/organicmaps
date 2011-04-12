#include "../../testing/testing.hpp"
#include "../sloynik_index.hpp"
#include "dictionary_mock.hpp"
#include "../../std/algorithm.hpp"
#include "../../std/bind.hpp"
#include "../../std/map.hpp"
#include "../../std/set.hpp"
#include "../../std/string.hpp"
#include "../../std/vector.hpp"
#include "../../std/stdio.hpp"

namespace
{
  set<sl::StrFn::Str const *> g_AllocatedStrSet;

  sl::StrFn::Str const * StrCreate(char const * utf8Data, uint32_t size)
  {
    sl::StrFn::Str const * res = reinterpret_cast<sl::StrFn::Str *>(new string(utf8Data, size));
    CHECK(g_AllocatedStrSet.insert(res).second, ());
    return res;
  }

  void StrDestroy(sl::StrFn::Str const * s)
  {
    if (s)
    {
      CHECK(g_AllocatedStrSet.count(s), ());
      g_AllocatedStrSet.erase(s);
      delete reinterpret_cast<string const *>(s);
    }
  }

  int StrSecondaryCompare(void *, sl::StrFn::Str const * pa, sl::StrFn::Str const * pb)
  {
    string const & a = *reinterpret_cast<string const *>(pa);
    string const & b = *reinterpret_cast<string const *>(pb);
    return a == b ? 0 : (a < b ? -1 : 1);
  }

  int StrPrimaryCompare(void *, sl::StrFn::Str const * pa, sl::StrFn::Str const * pb)
  {
    string s1(*reinterpret_cast<string const *>(pa));
    string s2(*reinterpret_cast<string const *>(pb));
    std::use_facet<std::ctype<char> >(std::locale()).tolower(&s1[0], &s1[0] + s1.size());
    std::use_facet<std::ctype<char> >(std::locale()).tolower(&s2[0], &s2[0] + s2.size());
    return s1 == s2 ? 0 : (s1 < s2 ? -1 : 1);
  }

  sl::StrFn StrFnForTest()
  {
    sl::StrFn strFn;
    strFn.Create = StrCreate;
    strFn.Destroy = StrDestroy;
    strFn.PrimaryCompare = StrPrimaryCompare;
    strFn.SecondaryCompare = StrSecondaryCompare;
    strFn.m_PrimaryCompareId = 1;
    strFn.m_SecondaryCompareId = 2;
    strFn.m_pData = NULL;
    return strFn;
  }

  void PushBackArticleIntoVector(vector<string> & v,
                                 sl::Dictionary const * pDic,
                                 sl::Dictionary::Id id)
  {
    v.push_back("");
    pDic->ArticleById(id, v.back());
  }

  string KeyByIndexId(sl::SortedIndex const & idx, sl::SortedIndex::Pos id)
  {
    string key;
    idx.GetDictionary().KeyById(idx.KeyIdByPos(id), key);
    return key;
  }

  string ArticleByIndexId(sl::SortedIndex const & idx, sl::SortedIndex::Pos id)
  {
    string article;
    idx.GetDictionary().ArticleById(idx.KeyIdByPos(id), article);
    return article;
  }

  void SetupDictionary(DictionaryMock & dictionary)
  {
    dictionary.Add("Hello", "Hello0"); // 0
    dictionary.Add("hello", "hello0"); // 1
    dictionary.Add("World", "World0"); // 2
    dictionary.Add("He", "He0");       // 3
    dictionary.Add("abc", "abc1");     // 4
  }
}

UNIT_TEST(SortedIndex_Smoke)
{
  string const & filePrefix = "sorted_index_smoke_test";
  DictionaryMock dictionary;
  SetupDictionary(dictionary);
  sl::StrFn strFn = StrFnForTest();
  sl::SortedIndex::Build(dictionary, strFn, filePrefix);
  sl::SortedIndex idx(dictionary, new FileReader(filePrefix + ".idx"), strFn);
  TEST_EQUAL(dictionary.KeyCount(), 5, ());
  TEST_EQUAL(KeyByIndexId(idx, 0), "abc", ());
  TEST_EQUAL(KeyByIndexId(idx, 1), "He", ());
  TEST_EQUAL(KeyByIndexId(idx, 2), "Hello", ());
  TEST_EQUAL(KeyByIndexId(idx, 3), "hello", ());
  TEST_EQUAL(KeyByIndexId(idx, 4), "World", ());
  TEST_EQUAL(ArticleByIndexId(idx, 0), "abc1", ());
  TEST_EQUAL(ArticleByIndexId(idx, 1), "He0", ());
  TEST_EQUAL(ArticleByIndexId(idx, 2), "Hello0", ());
  TEST_EQUAL(ArticleByIndexId(idx, 3), "hello0", ());
  TEST_EQUAL(ArticleByIndexId(idx, 4), "World0", ());
  TEST_EQUAL(idx.PrefixSearch(""), 0, ());
  TEST_EQUAL(idx.PrefixSearch("h"), 1, ());
  TEST_EQUAL(idx.PrefixSearch("H"), 1, ());
  TEST_EQUAL(idx.PrefixSearch("He"), 1, ());
  TEST_EQUAL(idx.PrefixSearch("he"), 1, ());
  TEST_EQUAL(idx.PrefixSearch("hea"), 2, ());
  TEST_EQUAL(idx.PrefixSearch("hel"), 2, ());
  TEST_EQUAL(idx.PrefixSearch("Hello"), 2, ());
  TEST_EQUAL(idx.PrefixSearch("W"), 4, ());
  TEST_EQUAL(idx.PrefixSearch("zzz"), 5, ());
  remove((filePrefix + ".idx").c_str());
  remove((filePrefix + ".idx").c_str());
  TEST(g_AllocatedStrSet.empty(), ());
}
