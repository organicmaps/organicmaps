#include "../../testing/testing.hpp"
#include "../slof_indexer.hpp"

#include "../../coding/coding_tests/compressor_test_utils.hpp"

#include "../../words/slof_dictionary.hpp"
#include "../../words/sloynik_engine.hpp"
#include "../../coding/reader.hpp"
#include "../../coding/writer.hpp"
#include "../../base/logging.hpp"
#include "../../base/macros.hpp"
#include "../../std/string.hpp"
#include "../../std/vector.hpp"

namespace
{
  string Key(sl::SlofDictionary const & dic, sl::Dictionary::Id id)
  {
    string res;
    dic.KeyById(id, res);
    return res;
  }

  string Article(sl::SlofDictionary const & dic, sl::Dictionary::Id id)
  {
    string res;
    dic.ArticleById(id, res);
    return res;
  }
}

UNIT_TEST(SlofIndexerEmptyTest)
{
  string serializedDictionary;
  {
    MemWriter<string> writer(serializedDictionary);
    sl::SlofIndexer indexer(writer, 20, &coding::TestCompressor);
  }
  sl::SlofDictionary dic(new MemReader(&serializedDictionary[0], serializedDictionary.size()),
                         &coding::TestDecompressor);
  TEST_EQUAL(dic.KeyCount(), 0, ());
}

UNIT_TEST(SlofIndexerSmokeTest)
{
  string serializedDictionary;
  {
    MemWriter<string> writer(serializedDictionary);
    sl::SlofIndexer indexer(writer, 25, &coding::TestCompressor);
    uint64_t articleM = indexer.AddArticle("ArticleM");
    indexer.AddKey("M", articleM);
    uint64_t articleHello = indexer.AddArticle("ArticleHello");
    indexer.AddKey("He", articleHello);
    uint64_t articleOk = indexer.AddArticle("ArticleOK");
    indexer.AddKey("OK", articleOk);
    indexer.AddKey("Hello", articleHello);
  }
  {
    sl::SlofDictionary dic(new MemReader(&serializedDictionary[0], serializedDictionary.size()),
                           &coding::TestDecompressor);
    TEST_EQUAL(dic.KeyCount(), 4, ());
    TEST_EQUAL(Key(dic, 0), "He", ());
    TEST_EQUAL(Key(dic, 1), "Hello", ());
    TEST_EQUAL(Key(dic, 2), "M", ());
    TEST_EQUAL(Key(dic, 3), "OK", ());
    TEST_EQUAL(Article(dic, 0), "ArticleHello", ());
    TEST_EQUAL(Article(dic, 1), "ArticleHello", ());
    TEST_EQUAL(Article(dic, 2), "ArticleM", ());
    TEST_EQUAL(Article(dic, 3), "ArticleOK", ());
  }
}

// TODO: Write end-to-end test (publisher-to-engine).
