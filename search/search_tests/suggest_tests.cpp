#include "testing/testing.hpp"

#include "search/string_utils.hpp"
#include "search/suggest.hpp"

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
