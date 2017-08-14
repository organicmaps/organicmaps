#include "testing/testing.hpp"

#include "partners_api/partners_api_tests/async_gui_thread.hpp"

#include "partners_api/viator_api.hpp"

#include <algorithm>
#include <random>
#include <sstream>
#include <vector>

#include "3party/jansson/myjansson.hpp"

using namespace partners_api;

namespace
{
UNIT_TEST(Viator_GetTopProducts)
{
  string result;
  viator::RawApi::GetTopProducts("684", "USD", 5, result);

  TEST(!result.empty(), ());

  my::Json root(result.c_str());
  bool success;
  FromJSONObjectOptionalField(root.get(), "success", success);
  TEST(success, ());
}

UNIT_CLASS_TEST(AsyncGuiThread, Viator_GetTop5Products)
{
  viator::Api api;
  std::string const kSofia = "5630";
  std::string resultId;
  std::vector<viator::Product> resultProducts;

  api.GetTop5Products(kSofia, "",
                      [&resultId, &resultProducts](std::string const & destId,
                                                   std::vector<viator::Product> const & products) {
                        resultId = destId;
                        resultProducts = products;

                        testing::Notify();
                      });

  testing::Wait();

  TEST_EQUAL(resultId, kSofia, ());
  TEST(!resultProducts.empty(), ());

  for (auto const & p : resultProducts)
    TEST_EQUAL(p.m_currency, "USD", ());

  resultId.clear();
  resultProducts.clear();

  api.GetTop5Products(kSofia, "RUB",
                      [&resultId, &resultProducts](std::string const & destId,
                                                   std::vector<viator::Product> const & products) {
                        resultId = destId;
                        resultProducts = products;

                        testing::Notify();
                      });

  testing::Wait();

  TEST_EQUAL(resultId, kSofia, ());
  TEST(!resultProducts.empty(), ());

  for (auto const & p : resultProducts)
    TEST_EQUAL(p.m_currency, "USD", ());

  resultId.clear();
  resultProducts.clear();

  api.GetTop5Products(kSofia, "GBP",
                      [&resultId, &resultProducts](std::string const & destId,
                                                   std::vector<viator::Product> const & products) {
                        resultId = destId;
                        resultProducts = products;

                        testing::Notify();
                      });

  testing::Wait();

  TEST_EQUAL(resultId, kSofia, ());
  TEST(!resultProducts.empty(), ());

  for (auto const & p : resultProducts)
    TEST_EQUAL(p.m_currency, "GBP", ());
}

UNIT_TEST(Viator_SortProducts)
{
  std::vector<viator::Product> products =
  {
    {"1", 10.0, 10, "", 10.0, "", "", "", ""},
    {"2", 9.0, 100, "", 200.0, "", "", "", ""},
    {"3", 9.0, 100, "", 100.0, "", "", "", ""},
    {"4", 9.0, 10, "", 200.0, "", "", "", ""},
    {"5", 9.0, 9, "", 300.0, "", "", "", ""},
    {"6", 8.0, 200, "", 400.0, "", "", "", ""},
    {"7", 7.0, 20, "", 200.0, "", "", "", ""},
    {"8", 7.0, 20, "", 1.0, "", "", "", ""},
    {"9", 7.0, 0, "", 300.0, "", "", "", ""},
    {"10", 7.0, 0, "", 1.0, "", "", "", ""}
  };

  for (size_t i = 0; i < 1000; ++i)
  {
    std::shuffle(products.begin(), products.end(), std::minstd_rand(std::minstd_rand::default_seed));
    viator::SortProducts(products);

    TEST_EQUAL(products[0].m_title, "1", ());
    TEST_EQUAL(products[1].m_title, "2", ());
    TEST_EQUAL(products[2].m_title, "3", ());
    TEST_EQUAL(products[3].m_title, "4", ());
    TEST_EQUAL(products[4].m_title, "5", ());
    TEST_EQUAL(products[5].m_title, "6", ());
    TEST_EQUAL(products[6].m_title, "7", ());
    TEST_EQUAL(products[7].m_title, "8", ());
    TEST_EQUAL(products[8].m_title, "9", ());
    TEST_EQUAL(products[9].m_title, "10", ());
  }
}
}  // namespace
