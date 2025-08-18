#include "testing/testing.hpp"

#include "search/geocoder_context.hpp"
#include "search/search_tests_support/helpers.hpp"
#include "search/search_tests_support/test_results_matching.hpp"
#include "search/tracer.hpp"

#include "generator/generator_tests_support/test_feature.hpp"

#include <memory>
#include <vector>

using namespace generator::tests_support;
using namespace search::tests_support;
using namespace search;
using namespace std;

namespace
{
class TracerTest : public SearchTest
{};

UNIT_CLASS_TEST(TracerTest, Smoke)
{
  using TokenType = BaseContext::TokenType;

  TestCity moscow(m2::PointD(0, 0), "Moscow", "en", 100 /* rank */);
  TestCafe regularCafe(m2::PointD(0, 0));
  TestCafe moscowCafe(m2::PointD(0, 0), "Moscow", "en");
  TestStreet tverskaya(vector<m2::PointD>{m2::PointD(0, 0), m2::PointD(0, 1)}, "Tverskaya street", "en");

  BuildWorld([&](TestMwmBuilder & builder) { builder.Add(moscow); });

  auto const id = BuildCountry("Wonderland", [&](TestMwmBuilder & builder)
  {
    builder.Add(regularCafe);
    builder.Add(moscowCafe);
    builder.Add(tverskaya);
  });

  SearchParams params;
  params.m_inputLocale = "en";
  params.m_viewport = m2::RectD(-1, -1, 1, 1);
  params.m_mode = Mode::Everywhere;

  {
    params.m_query = "moscow cafe";
    auto tracer = make_shared<Tracer>();
    params.m_tracer = tracer;

    TestSearchRequest request(m_engine, params);
    request.Run();
    Rules rules = {ExactMatch(id, regularCafe), ExactMatch(id, moscowCafe)};
    TEST(ResultsMatch(request.Results(), rules), ());

    auto const actual = tracer->GetUniqueParses();

    vector<Tracer::Parse> const expected{
        Tracer::Parse{{{TokenType::TOKEN_TYPE_SUBPOI, TokenRange(0, 2)}}, false /* category */},
        Tracer::Parse{
            {{TokenType::TOKEN_TYPE_CITY, TokenRange(0, 1)}, {TokenType::TOKEN_TYPE_SUBPOI, TokenRange(1, 2)}},
            true /* category */}};

    TEST_EQUAL(expected, actual, ());
  }

  {
    params.m_query = "moscow tverskaya 1 or 2";
    auto tracer = make_shared<Tracer>();
    params.m_tracer = tracer;

    TestSearchRequest request(m_engine, params);
    request.Run();
    TEST(ResultsMatch(request.Results(), {ExactMatch(id, tverskaya)}), ());

    auto const actual = tracer->GetUniqueParses();
    // Unrecognized tokens are not included into the parses.
    vector<Tracer::Parse> const expected{
        Tracer::Parse{{{TokenType::TOKEN_TYPE_STREET, TokenRange(1, 2)}}, false /* category */},
        Tracer::Parse{
            {{TokenType::TOKEN_TYPE_CITY, TokenRange(0, 1)}, {TokenType::TOKEN_TYPE_STREET, TokenRange(1, 2)}},
            false /* category */},
    };

    TEST_EQUAL(expected, actual, ());
  }
}
}  // namespace
