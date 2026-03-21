#include "testing/testing.hpp"

#include "search/geocoder_context.hpp"
#include "search/search_tests_support/helpers.hpp"
#include "search/search_tests_support/test_results_matching.hpp"
#include "search/tracer.hpp"

#include "generator/generator_tests_support/test_feature.hpp"

#include <memory>
#include <vector>

namespace
{
class TracerTest : public search::tests_support::SearchTest
{};

UNIT_CLASS_TEST(TracerTest, Smoke)
{
  using TokenType = search::BaseContext::TokenType;

  generator::tests_support::TestCity moscow(m2::PointD(0, 0), "Moscow", "en", 100 /* rank */);
  search::tests_support::TestCafe regularCafe(m2::PointD(0, 0));
  search::tests_support::TestCafe moscowCafe(m2::PointD(0, 0), "Moscow", "en");
  generator::tests_support::TestStreet tverskaya(std::vector<m2::PointD>{m2::PointD(0, 0), m2::PointD(0, 1)},
                                                 "Tverskaya street", "en");

  BuildWorld([&](generator::tests_support::TestMwmBuilder & builder) { builder.Add(moscow); });

  auto const id = BuildCountry("Wonderland", [&](generator::tests_support::TestMwmBuilder & builder)
  {
    builder.Add(regularCafe);
    builder.Add(moscowCafe);
    builder.Add(tverskaya);
  });

  search::SearchParams params;
  params.m_inputLocale = "en";
  params.m_viewport = m2::RectD(-1, -1, 1, 1);
  params.m_mode = search::Mode::Everywhere;

  {
    params.m_query = "moscow cafe";
    auto tracer = std::make_shared<search::Tracer>();
    params.m_tracer = tracer;

    search::tests_support::TestSearchRequest request(m_engine, params);
    request.Run();
    Rules rules = {search::tests_support::ExactMatch(id, regularCafe),
                   search::tests_support::ExactMatch(id, moscowCafe)};
    TEST(ResultsMatch(request.Results(), rules), ());

    auto const actual = tracer->GetUniqueParses();

    std::vector<search::Tracer::Parse> const expected{
        search::Tracer::Parse{{{TokenType::TOKEN_TYPE_SUBPOI, search::TokenRange(0, 2)}}, false /* category */},
        search::Tracer::Parse{{{TokenType::TOKEN_TYPE_CITY, search::TokenRange(0, 1)},
                               {TokenType::TOKEN_TYPE_SUBPOI, search::TokenRange(1, 2)}},
                              true /* category */}};

    TEST_EQUAL(expected, actual, ());
  }

  {
    params.m_query = "moscow tverskaya 1 or 2";
    auto tracer = std::make_shared<search::Tracer>();
    params.m_tracer = tracer;

    search::tests_support::TestSearchRequest request(m_engine, params);
    request.Run();
    TEST(ResultsMatch(request.Results(), {search::tests_support::ExactMatch(id, tverskaya)}), ());

    auto const actual = tracer->GetUniqueParses();
    // Unrecognized tokens are not included into the parses.
    std::vector<search::Tracer::Parse> const expected{
        search::Tracer::Parse{{{TokenType::TOKEN_TYPE_STREET, search::TokenRange(1, 2)}}, false /* category */},
        search::Tracer::Parse{{{TokenType::TOKEN_TYPE_CITY, search::TokenRange(0, 1)},
                               {TokenType::TOKEN_TYPE_STREET, search::TokenRange(1, 2)}},
                              false /* category */},
    };

    TEST_EQUAL(expected, actual, ());
  }
}
}  // namespace
