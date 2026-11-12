#include "testing/testing.hpp"

#include "generator/generator_tests_support/test_with_classificator.hpp"

#include "search/bookmarks/data.hpp"
#include "search/bookmarks/processor.hpp"
#include "search/emitter.hpp"

#include "indexer/classificator.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/cancellable.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace bookmarks_processor_tests
{
using namespace search::bookmarks;
using namespace search;
using namespace std;

using Ids = vector<Id>;

string const kLocale = "en";

class BookmarksProcessorTest : public generator::tests_support::TestWithClassificator
{
public:
  BookmarksProcessorTest() : m_processor(m_emitter, m_cancellable) {}

  Processor & GetProcessor() { return m_processor; }

  void Add(Id const & id, GroupId const & group, kml::BookmarkData const & data)
  {
    Doc const doc(data, kLocale);
    m_processor.Add(id, doc);
    AttachToGroup(id, group);
  }

  void Erase(Id const & id) { m_processor.Erase(id); }

  void Update(Id const & id, kml::BookmarkData const & data)
  {
    Doc const doc(data, kLocale);
    m_processor.Update(id, doc);
  }

  void AttachToGroup(Id const & id, GroupId const & group) { m_processor.AttachToGroup(id, group); }
  void DetachFromGroup(Id const & id, GroupId const & group) { m_processor.DetachFromGroup(id, group); }

  Ids Search(string const & query, GroupId const & groupId = kInvalidGroupId, bool requireAllTokens = false)
  {
    m_emitter.Init([](::search::Results const & /* results */) {} /* onResults */);

    vector<strings::UniString> tokens;
    auto const isPrefix = TokenizeStringAndCheckIfLastTokenIsPrefix(query, tokens);

    Processor::Params params;
    params.Init(query, tokens, isPrefix);

    params.m_groupId = groupId;
    params.m_requireAllTokens = requireAllTokens;

    m_processor.Search(params);
    Ids ids;
    for (auto const & result : m_emitter.GetResults().GetBookmarksResults())
      ids.emplace_back(result.m_id);
    return ids;
  }

  Ids SearchGlobal(string const & query) { return Search(query, kInvalidGroupId, true); }

protected:
  Emitter m_emitter;
  base::Cancellable m_cancellable;
  Processor m_processor;
};

kml::BookmarkData MakeBookmarkData(string const & name, string const & customName, string const & description,
                                   vector<string> const & types)
{
  kml::BookmarkData b;
  b.m_name = {{kml::kDefaultLangCode, name}};
  b.m_customName = {{kml::kDefaultLangCode, customName}};
  b.m_description = {{kml::kDefaultLangCode, description}};
  b.m_featureTypes.reserve(types.size());

  auto const & c = classif();
  for (auto const & typeString : types)
  {
    auto const t = c.GetTypeByPath(strings::Tokenize(typeString, "-"));
    TEST(t != Classificator::INVALID_TYPE, ());
    b.m_featureTypes.emplace_back(t);
  }

  return b;
}

UNIT_CLASS_TEST(BookmarksProcessorTest, FullNameMatchRanksBeforePartialMatch)
{
  GetProcessor().EnableIndexingOfBookmarkGroup(GroupId{0}, true);
  Add(Id{10}, GroupId{0}, MakeBookmarkData("Maya", "", "", {}));
  Add(Id{12}, GroupId{0}, MakeBookmarkData("Maya Patel", "", "", {}));
  auto const results = Search("maya patel");
  TEST_EQUAL(results.size(), 2, ());
  TEST_EQUAL(results.front(), Id{12}, ());
}

UNIT_CLASS_TEST(BookmarksProcessorTest, GlobalSearchRequiresEveryQueryToken)
{
  GetProcessor().EnableIndexingOfBookmarkGroup(GroupId{0}, true);
  GetProcessor().EnableIndexingOfBookmarkGroup(GroupId{1}, true);
  Add(Id{10}, GroupId{0}, MakeBookmarkData("Ben Example", "", "868 W 67 Avenue", {}));
  Add(Id{12}, GroupId{1}, MakeBookmarkData("868 W 67 Bay", "", "", {}));
  TEST_EQUAL(SearchGlobal("868 W 67 B"), Ids({12}), ());
  TEST_EQUAL(SearchGlobal("868 W 67 B "), Ids{}, ());
  TEST_EQUAL(SearchGlobal("868 W 68 B"), Ids{}, ());
  TEST_EQUAL(SearchGlobal("nonexistent Ben"), Ids{}, ());
}

UNIT_CLASS_TEST(BookmarksProcessorTest, GlobalSearchRetainsFuzzyNamesAndPrefixes)
{
  GetProcessor().EnableIndexingOfBookmarkGroup(GroupId{0}, true);
  Add(Id{10}, GroupId{0}, MakeBookmarkData("Maya", "", "", {}));
  Add(Id{12}, GroupId{0}, MakeBookmarkData("Maya Patel", "", "", {}));
  TEST_EQUAL(SearchGlobal("maya pat"), Ids({12}), ());
  TEST_EQUAL(SearchGlobal("maya patal "), Ids({12}), ());
  TEST_EQUAL(SearchGlobal("patel maya "), Ids({12}), ());
  TEST_EQUAL(Search("maya pat", GroupId{0}).size(), 2, ());
}

UNIT_CLASS_TEST(BookmarksProcessorTest, Smoke)
{
  GetProcessor().EnableIndexingOfDescriptions(true);

  Add(Id{10}, GroupId{0},
      MakeBookmarkData("Double R Diner" /* name */, "2R Diner" /* customName */,
                       "They've got a cherry pie there that'll kill ya!" /* description */,
                       {"amenity-cafe"} /* types */));

  Add(Id{18}, GroupId{0},
      MakeBookmarkData("Silver Mustang Casino" /* name */, "Ag Mustang" /* customName */,
                       "Joyful place, owners Bradley and Rodney are very friendly!" /* description */,
                       {"amenity-casino"} /* types */));
  Add(Id{20}, GroupId{1},
      MakeBookmarkData("Great Northern Hotel" /* name */, "N Hotel" /* customName */,
                       "Clean place with a reasonable price" /* description */, {"tourism-hotel"} /* types */));

  TEST_EQUAL(Search("R&R food"), Ids{}, ());
  GetProcessor().EnableIndexingOfBookmarkGroup(GroupId{0}, true /* enable */);
  TEST_EQUAL(Search("R&R food"), Ids({10}), ());
  GetProcessor().EnableIndexingOfBookmarkGroup(GroupId{0}, false /* enable */);
  TEST_EQUAL(Search("R&R food"), Ids{}, ());
  GetProcessor().EnableIndexingOfBookmarkGroup(GroupId{0}, true /* enable */);
  TEST_EQUAL(Search("R&R food"), Ids({10}), ());

  GetProcessor().EnableIndexingOfBookmarkGroup(GroupId{1}, true /* enable */);

  TEST_EQUAL(Search("cherry pie"), Ids({10}), ());
  TEST_EQUAL(Search("great silver hotel"), Ids({20, 18}), ());
  TEST_EQUAL(Search("double r cafe"), Ids({10}), ());
  TEST_EQUAL(Search("dine"), Ids({10}), ());
  TEST_EQUAL(Search("2R"), Ids({10}), ());
  TEST_EQUAL(Search("Ag"), Ids({18}), ());

  TEST_EQUAL(Search("place"), Ids({20, 18}), ());
  TEST_EQUAL(Search("place", GroupId{0}), Ids({18}), ());
  DetachFromGroup(Id{20}, GroupId{1});
  AttachToGroup(Id{20}, GroupId{0});
  TEST_EQUAL(Search("place", GroupId{0}), Ids({20, 18}), ());

  Update(20, MakeBookmarkData("Great Northern Hotel" /* name */, "N Hotel" /* customName */,
                              "Clean establishment with a reasonable price" /* description */,
                              {"tourism-hotel"} /* types */));
  TEST_EQUAL(Search("place", GroupId{0}), Ids({18}), ());

  GetProcessor().Reset();
  TEST_EQUAL(Search("place", GroupId{0}), Ids{}, ());
}

UNIT_CLASS_TEST(BookmarksProcessorTest, GlobalNameSearchIncludesAllListsAndUpdates)
{
  GetProcessor().EnableIndexingOfBookmarkGroup(GroupId{1}, true);
  GetProcessor().EnableIndexingOfBookmarkGroup(GroupId{2}, true);
  Add(Id{10}, GroupId{1}, MakeBookmarkData("Maya Patel", "", "", {}));
  Add(Id{12}, GroupId{2}, MakeBookmarkData("Maya Park", "", "", {}));
  auto results = Search("maya");
  std::sort(results.begin(), results.end());
  TEST_EQUAL(results, Ids({10, 12}), ());
  TEST_EQUAL(Search("maya", GroupId{1}), Ids({10}), ());
  Update(Id{10}, MakeBookmarkData("Jordan Patel", "", "", {}));
  TEST_EQUAL(Search("maya"), Ids({12}), ());
  DetachFromGroup(Id{12}, GroupId{2});
  Erase(Id{12});
  TEST(Search("maya").empty(), ());
}

UNIT_CLASS_TEST(BookmarksProcessorTest, SearchByType)
{
  GetProcessor().EnableIndexingOfDescriptions(true);
  GetProcessor().EnableIndexingOfBookmarkGroup(GroupId{0}, true /* enable */);

  Add(Id{10}, GroupId{0},
      MakeBookmarkData("Double R Diner" /* name */, "2R Diner" /* customName */,
                       "They've got a cherry pie there that'll kill ya!" /* description */,
                       {"amenity-cafe"} /* types */));

  Add(Id{12}, GroupId{0},
      MakeBookmarkData("" /* name */, "" /* customName */, "" /* description */, {"amenity-cafe"} /* types */));

  Add(Id{0}, GroupId{0}, MakeBookmarkData("" /* name */, "" /* customName */, "" /* description */, {} /* types */));

  TEST_EQUAL(Search("cafe", GroupId{0}), Ids({12, 10}), ());
  TEST_EQUAL(Search("кафе", GroupId{0}), Ids{}, ());
  TEST_EQUAL(Search("", GroupId{0}), Ids{}, ());
}

UNIT_CLASS_TEST(BookmarksProcessorTest, IndexDescriptions)
{
  GetProcessor().EnableIndexingOfDescriptions(true);
  GetProcessor().EnableIndexingOfBookmarkGroup(GroupId{0}, true /* enable */);

  Add(Id{10}, GroupId{0},
      MakeBookmarkData("Double R Diner" /* name */, "2R Diner" /* customName */,
                       "They've got a cherry pie there that'll kill ya!" /* description */,
                       {"amenity-cafe"} /* types */));
  TEST_EQUAL(Search("diner"), Ids({10}), ());
  TEST_EQUAL(Search("cherry pie"), Ids({10}), ());

  DetachFromGroup(Id{10}, GroupId{0});
  Erase(Id{10});
  TEST_EQUAL(Search("diner"), Ids{}, ());
  TEST_EQUAL(Search("cherry pie"), Ids{}, ());

  GetProcessor().EnableIndexingOfDescriptions(false);
  Add(Id{10}, GroupId{0},
      MakeBookmarkData("Double R Diner" /* name */, "2R Diner" /* customName */,
                       "They've got a cherry pie there that'll kill ya!" /* description */,
                       {"amenity-cafe"} /* types */));
  TEST_EQUAL(Search("diner"), Ids({10}), ());
  TEST_EQUAL(Search("cherry pie"), Ids{}, ());

  // Results for already indexed bookmarks don't change.
  GetProcessor().EnableIndexingOfDescriptions(true);
  TEST_EQUAL(Search("diner"), Ids({10}), ());
  TEST_EQUAL(Search("cherry pie"), Ids{}, ());

  DetachFromGroup(Id{10}, GroupId{0});
  Erase(Id{10});
  TEST_EQUAL(Search("diner"), Ids{}, ());
  TEST_EQUAL(Search("cherry pie"), Ids{}, ());
}

}  // namespace bookmarks_processor_tests
