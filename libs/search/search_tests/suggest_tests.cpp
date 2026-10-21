#include "testing/testing.hpp"

#include "search/search_tests_support/test_search_engine.hpp"
#include "search/search_tests_support/test_search_request.hpp"
#include "search/string_utils.hpp"
#include "search/suggest.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"

#include <algorithm>
#include <string>
#include <vector>

UNIT_TEST(Suggest_Smoke)
{
  std::string const street = "Avenida Santa Fe";
  TEST_EQUAL(street + ' ', search::GetSuggestion(street, search::MakeQueryString("santa f")), ());
  TEST_EQUAL("3655 " + street + ' ', search::GetSuggestion(street, search::MakeQueryString("3655 santa f")), ());
  TEST_EQUAL("3655 east " + street + ' ', search::GetSuggestion(street, search::MakeQueryString("3655 santa east f")),
             ());

  // Full prefix match -> no suggest.
  TEST_EQUAL("", search::GetSuggestion(street, search::MakeQueryString("east santa fe")), ());

  /// @todo Process street shorts like: st, av, ne, w, ..
  // TEST_EQUAL(street, search::GetSuggestion(street, search::MakeQueryString("av sant")), ());
}

UNIT_TEST(Suggest_CategoryPreservesSpelling)
{
  classificator::Load();

  FrozenDataSource dataSource;
  search::tests_support::TestSearchEngine engine(dataSource, search::Engine::Params("cy", 1), true);
  search::SearchParams params;
  params.m_inputLocale = "cy";
  params.m_viewport = {-1, -1, 1, 1};
  params.m_suggestsEnabled = true;

  for (auto const * query : {"cara", "CARAFÁ"})
  {
    params.m_query = query;
    search::tests_support::TestSearchRequest request(engine, params);
    request.Run();

    auto const count =
        std::count_if(request.Results().begin(), request.Results().end(), [](search::Result const & result)
    { return result.IsSuggest() && result.GetString() == "Carafán" && result.GetSuggestionString() == "Carafán "; });
    TEST_EQUAL(count, 1, (query));
  }

  // Accent-insensitive matching must still suppress suggestions for an already complete category name.
  for (auto const * query : {"carafan", "Carafán"})
  {
    params.m_query = query;
    search::tests_support::TestSearchRequest request(engine, params);
    request.Run();
    TEST(std::none_of(request.Results().begin(), request.Results().end(), [](search::Result const & result)
    { return result.IsSuggest() && result.GetString() == "Carafán"; }),
         (query));
  }
}

UNIT_TEST(Suggest_CategoryOrder)
{
  classificator::Load();

  FrozenDataSource dataSource;
  search::tests_support::TestSearchEngine engine(dataSource, search::Engine::Params("en", 1), true);
  search::SearchParams params;
  params.m_query = "car";
  params.m_inputLocale = "en";
  params.m_viewport = {-1, -1, 1, 1};
  params.m_suggestsEnabled = true;

  search::tests_support::TestSearchRequest request(engine, params);
  request.Run();

  std::vector<std::string> suggestions;
  for (auto const & result : request.Results())
    if (result.IsSuggest())
      suggestions.push_back(result.GetString());

  // Ordering must precede the five-result limit so category traversal does not change the selected suggestions.
  std::vector<std::string> const expected = {"Car Charger", "Car Dealership", "Car Parts", "Car Rental",
                                             "Car Repair Workshop"};
  TEST_EQUAL(suggestions, expected, ());
}
