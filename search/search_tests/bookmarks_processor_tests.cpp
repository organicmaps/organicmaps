#include "testing/testing.hpp"

#include "search/bookmarks/data.hpp"
#include "search/bookmarks/processor.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include <string>
#include <vector>

using namespace search::bookmarks;
using namespace search;
using namespace std;

namespace
{
class BookmarksProcessorTest
{
public:
  using Id = Processor::Id;
  using Doc = Processor::Doc;

  void Add(Id const & id, Doc const & doc) { m_processor.Add(id, doc); }
  void Erase(Id const & id, Doc const & doc) { m_processor.Erase(id, doc); }

  vector<Id> Search(string const & query) const
  {
    vector<strings::UniString> tokens;
    auto const isPrefix =
        TokenizeStringAndCheckIfLastTokenIsPrefix(query, tokens, search::Delimiters());

    QueryParams params;
    if (isPrefix)
    {
      ASSERT(!tokens.empty(), ());
      params.InitWithPrefix(tokens.begin(), tokens.end() - 1, tokens.back());
    }
    else
    {
      params.InitNoPrefix(tokens.begin(), tokens.end());
    }
    return m_processor.Search(params);
  }

protected:
  Processor m_processor;
};

UNIT_CLASS_TEST(BookmarksProcessorTest, Smoke)
{
  Add(10, {"Double R Diner" /* name */,
           "They've got a cherry pie there that'll kill ya!" /* description */, "Food" /* type */});

  Add(18, {"Silver Mustang Casino" /* name */,
           "Joyful place, owners Bradley and Rodney are very friendly!", "Entertainment"});
  Add(20, {"Great Northern Hotel" /* name */,
           "Clean place with a reasonable price" /* description */, "Hotel" /* type */});

  TEST_EQUAL(Search("R&R food"), vector<Id>({10}), ());
  TEST_EQUAL(Search("cherry pie"), vector<Id>({10}), ());
  TEST_EQUAL(Search("great silver hotel"), vector<Id>({20, 18}), ());
  TEST_EQUAL(Search("double r cafe"), vector<Id>({10}), ());
  TEST_EQUAL(Search("dine"), vector<Id>({10}), ());
}
}  // namespace
