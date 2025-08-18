#include "testing/testing.hpp"

#include "search/base/text_index/mem.hpp"
#include "search/base/text_index/merger.hpp"
#include "search/base/text_index/reader.hpp"
#include "search/base/text_index/text_index.hpp"

#include "indexer/search_string_utils.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_writer.hpp"
#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

#include <boost/iterator/transform_iterator.hpp>

namespace text_index_tests
{
using namespace platform::tests_support;
using namespace search_base;
using namespace search;
using namespace std;

using boost::make_transform_iterator;

// Prepend several bytes to serialized indexes in order to check the relative offsets.
size_t const kSkip = 10;

search_base::MemTextIndex BuildMemTextIndex(vector<string> const & docsCollection)
{
  MemTextIndex memIndex;

  for (size_t docId = 0; docId < docsCollection.size(); ++docId)
  {
    strings::Tokenize(docsCollection[docId], " ", [&memIndex, docId](std::string_view tok)
    { memIndex.AddPosting(std::string(tok), static_cast<uint32_t>(docId)); });
  }

  return memIndex;
}

void Serdes(MemTextIndex & memIndex, MemTextIndex & deserializedMemIndex, vector<uint8_t> & buf)
{
  buf.clear();
  {
    MemWriter<vector<uint8_t>> writer(buf);
    WriteZeroesToSink(writer, kSkip);
    memIndex.Serialize(writer);
  }

  {
    MemReaderWithExceptions reader(buf.data() + kSkip, buf.size());
    ReaderSource<decltype(reader)> source(reader);
    deserializedMemIndex.Deserialize(source);
  }
}

template <typename Index, typename Token>
void TestForEach(Index const & index, Token const & token, vector<uint32_t> const & expected)
{
  vector<uint32_t> actual;
  index.ForEachPosting(token, base::MakeBackInsertFunctor(actual));
  TEST_EQUAL(actual, expected, (token));
}

UNIT_TEST(TextIndex_Smoke)
{
  vector<search_base::Token> const docsCollection = {
      "a b c",
      "a c",
  };

  auto memIndex = BuildMemTextIndex(docsCollection);

  vector<uint8_t> indexData;
  MemTextIndex deserializedMemIndex;
  Serdes(memIndex, deserializedMemIndex, indexData);

  for (auto const & index : {memIndex, deserializedMemIndex})
  {
    TestForEach(index, "a", {0, 1});
    TestForEach(index, "b", {0});
    TestForEach(index, "c", {0, 1});
    TestForEach(index, "d", {});
  }

  {
    string contents;
    copy_n(indexData.begin() + kSkip, indexData.size() - kSkip, back_inserter(contents));
    ScopedFile file("text_index_tmp", contents);
    FileReader fileReader(file.GetFullPath());
    TextIndexReader textIndexReader(fileReader);
    TestForEach(textIndexReader, "a", {0, 1});
    TestForEach(textIndexReader, "b", {0});
    TestForEach(textIndexReader, "c", {0, 1});
    TestForEach(textIndexReader, "d", {});
  }
}

UNIT_TEST(TextIndex_UniString)
{
  vector<std::string> const docsCollectionUtf8s = {
      "â b ç",
      "â ç",
  };
  vector<strings::UniString> const docsCollection(
      make_transform_iterator(docsCollectionUtf8s.begin(), &strings::MakeUniString),
      make_transform_iterator(docsCollectionUtf8s.end(), &strings::MakeUniString));

  MemTextIndex memIndex;

  for (size_t docId = 0; docId < docsCollection.size(); ++docId)
  {
    auto addToIndex = [&](strings::UniString const & token)
    { memIndex.AddPosting(strings::ToUtf8(token), static_cast<uint32_t>(docId)); };
    auto delims = [](strings::UniChar const & c) { return c == ' '; };
    SplitUniString(docsCollection[docId], addToIndex, delims);
  }

  vector<uint8_t> indexData;
  MemTextIndex deserializedMemIndex;
  Serdes(memIndex, deserializedMemIndex, indexData);

  for (auto const & index : {memIndex, deserializedMemIndex})
  {
    TestForEach(index, strings::MakeUniString("a"), {});
    TestForEach(index, strings::MakeUniString("â"), {0, 1});
    TestForEach(index, strings::MakeUniString("b"), {0});
    TestForEach(index, strings::MakeUniString("ç"), {0, 1});
  }
}

UNIT_TEST(TextIndex_Merging)
{
  // todo(@m) Arrays? docsCollection[i]
  vector<search_base::Token> const docsCollection1 = {
      "a b c",
      "",
      "d",
  };
  vector<search_base::Token> const docsCollection2 = {
      "",
      "a c",
      "e",
  };

  auto memIndex1 = BuildMemTextIndex(docsCollection1);
  vector<uint8_t> indexData1;
  MemTextIndex deserializedMemIndex1;
  Serdes(memIndex1, deserializedMemIndex1, indexData1);

  auto memIndex2 = BuildMemTextIndex(docsCollection2);
  vector<uint8_t> indexData2;
  MemTextIndex deserializedMemIndex2;
  Serdes(memIndex2, deserializedMemIndex2, indexData2);

  {
    string contents1;
    copy_n(indexData1.begin() + kSkip, indexData1.size() - kSkip, back_inserter(contents1));
    ScopedFile file1("text_index_tmp1", contents1);
    FileReader fileReader1(file1.GetFullPath());
    TextIndexReader textIndexReader1(fileReader1);

    string contents2;
    copy_n(indexData2.begin() + kSkip, indexData2.size() - kSkip, back_inserter(contents2));
    ScopedFile file2("text_index_tmp2", contents2);
    FileReader fileReader2(file2.GetFullPath());
    TextIndexReader textIndexReader2(fileReader2);

    ScopedFile file3("text_index_tmp3", ScopedFile::Mode::Create);
    {
      FileWriter fileWriter(file3.GetFullPath());
      TextIndexMerger::Merge(textIndexReader1, textIndexReader2, fileWriter);
    }

    FileReader fileReader3(file3.GetFullPath());
    TextIndexReader textIndexReader3(fileReader3);
    TestForEach(textIndexReader3, "a", {0, 1});
    TestForEach(textIndexReader3, "b", {0});
    TestForEach(textIndexReader3, "c", {0, 1});
    TestForEach(textIndexReader3, "x", {});
    TestForEach(textIndexReader3, "d", {2});
    TestForEach(textIndexReader3, "e", {2});
  }
}
}  // namespace text_index_tests
