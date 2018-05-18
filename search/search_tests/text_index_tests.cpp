#include "testing/testing.hpp"

#include "search/base/text_index.hpp"

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"
#include "coding/writer.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

using namespace search::base;
using namespace search;
using namespace std;

UNIT_TEST(MemTextIndex_Smoke)
{
  vector<string> const docsCollection = {
      "a b c",
      "a c",
  };

  MemTextIndex<string> memIndex;

  for (size_t docId = 0; docId < docsCollection.size(); ++docId)
  {
    strings::SimpleTokenizer tok(docsCollection[docId], " ");
    while (tok)
    {
      memIndex.AddPosting(*tok, static_cast<uint32_t>(docId));
      ++tok;
    }
  }

  // Prepend several bytes to check the relative offsets.
  size_t const kSkip = 10;
  vector<uint8_t> buf;
  {
    MemWriter<decltype(buf)> writer(buf);
    WriteZeroesToSink(writer, kSkip);
    memIndex.Serialize(writer);
  }

  MemTextIndex<string> deserializedMemIndex;
  {
    MemReaderWithExceptions reader(buf.data() + kSkip, buf.size());
    ReaderSource<decltype(reader)> source(reader);
    deserializedMemIndex.Deserialize(source);
  }

  auto testForEach = [&](string const & token, vector<uint32_t> const & expected) {
    for (auto const & idx : {memIndex, deserializedMemIndex})
    {
      vector<uint32_t> actual;
      idx.ForEachPosting(token, MakeBackInsertFunctor(actual));
      TEST_EQUAL(actual, expected, ());
    }
  };

  testForEach("a", {0, 1});
  testForEach("b", {0});
  testForEach("c", {0, 1});
}
