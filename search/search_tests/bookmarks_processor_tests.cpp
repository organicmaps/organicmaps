#include "testing/testing.hpp"

#include "search/bookmarks/data.hpp"
#include "search/bookmarks/processor.hpp"
#include "search/emitter.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/cancellable.hpp"

#include <string>
#include <vector>

using namespace search::bookmarks;
using namespace search;
using namespace std;

namespace
{
using Ids = vector<Id>;

class BookmarksProcessorTest
{
public:
  BookmarksProcessorTest() : m_processor(m_emitter, m_cancellable) {}

  void Add(Id const & id, Doc const & doc) { m_processor.Add(id, doc); }

  void Erase(Id const & id) { m_processor.Erase(id); }

  Ids Search(string const & query)
  {
    m_emitter.Init([](::search::Results const & /* results */) {} /* onResults */);

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

    m_processor.Search(params);
    Ids ids;
    for (auto const & result : m_emitter.GetResults().GetBookmarksResults())
      ids.emplace_back(result.m_id);
    return ids;
  }

protected:
  Emitter m_emitter;
  my::Cancellable m_cancellable;
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

  TEST_EQUAL(Search("R&R food"), Ids({10}), ());
  TEST_EQUAL(Search("cherry pie"), Ids({10}), ());
  TEST_EQUAL(Search("great silver hotel"), Ids({20, 18}), ());
  TEST_EQUAL(Search("double r cafe"), Ids({10}), ());
  TEST_EQUAL(Search("dine"), Ids({10}), ());
}
}  // namespace
