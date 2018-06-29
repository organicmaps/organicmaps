#include "testing/testing.hpp"

#include "search/base/text_index.hpp"

#include "indexer/search_string_utils.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "std/transform_iterator.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

using namespace platform::tests_support;
using namespace search::base;
using namespace search;
using namespace std;

namespace
{
// Prepend several bytes to serialized indexes in order to check the relative offsets.
size_t const kSkip = 10;

template <typename Token>
void Serdes(MemTextIndex<Token> & memIndex, MemTextIndex<Token> & deserializedMemIndex,
            vector<uint8_t> & buf)
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
  index.ForEachPosting(token, MakeBackInsertFunctor(actual));
  TEST_EQUAL(actual, expected, ());
};
}  // namespace

namespace search
{
UNIT_TEST(TextIndex_Smoke)
{
  using Token = string;

  vector<Token> const docsCollection = {
      "a b c",
      "a c",
  };

  MemTextIndex<Token> memIndex;

  for (size_t docId = 0; docId < docsCollection.size(); ++docId)
  {
    strings::SimpleTokenizer tok(docsCollection[docId], " ");
    while (tok)
    {
      memIndex.AddPosting(*tok, static_cast<uint32_t>(docId));
      ++tok;
    }
  }

  vector<uint8_t> indexData;
  MemTextIndex<Token> deserializedMemIndex;
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
    TextIndexReader<Token> textIndexReader(fileReader);
    TestForEach(textIndexReader, "a", {0, 1});
    TestForEach(textIndexReader, "b", {0});
    TestForEach(textIndexReader, "c", {0, 1});
    TestForEach(textIndexReader, "d", {});
  }
}

UNIT_TEST(TextIndex_UniString)
{
  using Token = strings::UniString;

  vector<std::string> const docsCollectionUtf8s = {
      "â b ç",
      "â ç",
  };
  vector<Token> const docsCollection(
      make_transform_iterator(docsCollectionUtf8s.begin(), &strings::MakeUniString),
      make_transform_iterator(docsCollectionUtf8s.end(), &strings::MakeUniString));

  MemTextIndex<Token> memIndex;

  for (size_t docId = 0; docId < docsCollection.size(); ++docId)
  {
    auto addToIndex = [&](Token const & token) {
      memIndex.AddPosting(token, static_cast<uint32_t>(docId));
    };
    auto delims = [](strings::UniChar const & c) { return c == ' '; };
    SplitUniString(docsCollection[docId], addToIndex, delims);
  }

  vector<uint8_t> indexData;
  MemTextIndex<Token> deserializedMemIndex;
  Serdes(memIndex, deserializedMemIndex, indexData);

  for (auto const & index : {memIndex, deserializedMemIndex})
  {
    TestForEach(index, strings::MakeUniString("a"), {});
    TestForEach(index, strings::MakeUniString("â"), {0, 1});
    TestForEach(index, strings::MakeUniString("b"), {0});
    TestForEach(index, strings::MakeUniString("ç"), {0, 1});
  }
}
}  // namespace search
