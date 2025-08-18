#include "testing/testing.hpp"

#include "search/search_tests_support/helpers.hpp"

namespace benchmark_tests
{

using BenchmarkFixture = search::tests_support::SearchTest;

UNIT_CLASS_TEST(BenchmarkFixture, Smoke)
{
  RegisterLocalMapsInViewport(mercator::Bounds::FullRect());

  SetViewport({50.1052, 8.6868}, 10000);  // Frankfurt am Main

  auto request = MakeRequest("b");
  LOG(LINFO, (request->ResponseTime().count()));
}

}  // namespace benchmark_tests
