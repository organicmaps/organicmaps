#include "testing/testing.hpp"

#include "search/base/text_index.hpp"

#include "indexer/search_string_utils.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "std/transform_iterator.hpp"

#include <cstdint>
#include <string>
#include <vector>

using namespace search::base;
using namespace search;
using namespace std;

namespace
{
template <typename Token>
void Serdes(MemTextIndex<Token> & memIndex, MemTextIndex<Token> & deserializedMemIndex)
{
  // Prepend several bytes to check the relative offsets.
  size_t const kSkip = 10;
  vector<uint8_t> buf;
  {
    MemWriter<decltype(buf)> writer(buf);
    WriteZeroesToSink(writer, kSkip);
    memIndex.Serialize(writer);
  }

  {
    MemReaderWithExceptions reader(buf.data() + kSkip, buf.size());
    ReaderSource<decltype(reader)> source(reader);
    deserializedMemIndex.Deserialize(source);
  }
}

template <typename Token>
void TestForEach(MemTextIndex<Token> const & index, Token const & token,
                 vector<uint32_t> const & expected)
{
  vector<uint32_t> actual;
  index.ForEachPosting(token, MakeBackInsertFunctor(actual));
  TEST_EQUAL(actual, expected, ());
};
}  // namespace

namespace search
{
UNIT_TEST(MemTextIndex_Smoke)
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

  MemTextIndex<Token> deserializedMemIndex;
  Serdes(memIndex, deserializedMemIndex);

  for (auto const & index : {memIndex, deserializedMemIndex})
  {
    TestForEach<Token>(index, "a", {0, 1});
    TestForEach<Token>(index, "b", {0});
    TestForEach<Token>(index, "c", {0, 1});
  }
}

UNIT_TEST(MemTextIndex_UniString)
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

  MemTextIndex<Token> deserializedMemIndex;
  Serdes(memIndex, deserializedMemIndex);

  for (auto const & index : {memIndex, deserializedMemIndex})
  {
    TestForEach<Token>(index, strings::MakeUniString("a"), {});
    TestForEach<Token>(index, strings::MakeUniString("â"), {0, 1});
    TestForEach<Token>(index, strings::MakeUniString("b"), {0});
    TestForEach<Token>(index, strings::MakeUniString("ç"), {0, 1});
  }
}
}  // namespace search
