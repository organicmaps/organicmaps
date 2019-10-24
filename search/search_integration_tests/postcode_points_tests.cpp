#include "testing/testing.hpp"

#include "search/postcode_points.hpp"
#include "search/search_tests_support/helpers.hpp"

#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "storage/country_info_getter.hpp"

#include "indexer/search_string_utils.hpp"

#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/point_coding.hpp"

#include "base/file_name_utils.hpp"
#include "base/math.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace generator::tests_support;
using namespace platform::tests_support;
using namespace search::tests_support;
using namespace search;
using namespace std;

namespace
{
class PostcodePointsTest : public SearchTest
{
};

UNIT_CLASS_TEST(PostcodePointsTest, Smoke)
{
  string const countryName = "Wonderland";

  Platform & platform = GetPlatform();
  auto const & writableDir = platform.WritableDir();
  string const testFile = "postcodes.csv";
  auto const postcodesRelativePath = base::JoinPath(writableDir, testFile);

  ScopedFile const osmScopedFile(testFile,
                                 "aa11 0, 1.0, 1.0\n"
                                 "aa11 1, 2.0, 2.0\n"
                                 "aa11 2, 3.0, 3.0\n");

  auto infoGetter = std::make_shared<storage::CountryInfoGetterForTesting>();
  infoGetter->AddCountry(storage::CountryDef(
      countryName,
      m2::RectD(MercatorBounds::FromLatLon(0.99, 0.99), MercatorBounds::FromLatLon(3.01, 3.01))));

  auto const id = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.SetPostcodesData(postcodesRelativePath, infoGetter);
  });

  auto handle = m_dataSource.GetMwmHandleById(id);
  auto value = handle.GetValue();
  CHECK(value, ());
  TEST(value->m_cont.IsExist(POSTCODE_POINTS_FILE_TAG), ());

  PostcodePoints p(*value);
  {
    vector<m2::PointD> points;
    p.Get(NormalizeAndSimplifyString("aa11 0"), points);
    TEST_EQUAL(points.size(), 1, ());
    TEST(base::AlmostEqualAbs(points[0], MercatorBounds::FromLatLon(1.0, 1.0), kMwmPointAccuracy),
         ());
  }
  {
    vector<m2::PointD> points;
    p.Get(NormalizeAndSimplifyString("aa11 1"), points);
    TEST_EQUAL(points.size(), 1, ());
    TEST(base::AlmostEqualAbs(points[0], MercatorBounds::FromLatLon(2.0, 2.0), kMwmPointAccuracy),
         ());
  }
  {
    vector<m2::PointD> points;
    p.Get(NormalizeAndSimplifyString("aa11 2"), points);
    TEST_EQUAL(points.size(), 1, ());
    TEST(base::AlmostEqualAbs(points[0], MercatorBounds::FromLatLon(3.0, 3.0), kMwmPointAccuracy),
         ());
  }
  {
    vector<m2::PointD> points;
    p.Get(NormalizeAndSimplifyString("aa11"), points);
    TEST_EQUAL(points.size(), 3, ());
    sort(points.begin(), points.end());
    TEST(base::AlmostEqualAbs(points[0], MercatorBounds::FromLatLon(1.0, 1.0), kMwmPointAccuracy),
         ());
    TEST(base::AlmostEqualAbs(points[1], MercatorBounds::FromLatLon(2.0, 2.0), kMwmPointAccuracy),
         ());
    TEST(base::AlmostEqualAbs(points[2], MercatorBounds::FromLatLon(3.0, 3.0), kMwmPointAccuracy),
         ());
  }
}

UNIT_CLASS_TEST(PostcodePointsTest, SearchPostcode)
{
  string const countryName = "Wonderland";

  Platform & platform = GetPlatform();
  auto const & writableDir = platform.WritableDir();
  string const testFile = "postcodes.csv";
  auto const postcodesRelativePath = base::JoinPath(writableDir, testFile);

  // Use the same latitude for easier center calculation.
  ScopedFile const osmScopedFile(testFile,
                                 "BA6 7JP, 5.0, 4.0\n"
                                 "BA6 8JP, 5.0, 6.0\n");

  auto infoGetter = std::make_shared<storage::CountryInfoGetterForTesting>();
  infoGetter->AddCountry(storage::CountryDef(
      countryName,
      m2::RectD(MercatorBounds::FromLatLon(3.0, 3.0), MercatorBounds::FromLatLon(7.0, 7.0))));

  auto const id = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.SetPostcodesData(postcodesRelativePath, infoGetter);
  });

  auto test = [&](string const & query, m2::PointD const & expected) {
    auto request = MakeRequest(query);
    auto const & results = request->Results();
    TEST_EQUAL(results.size(), 1, ());

    auto const & result = results[0];
    TEST_EQUAL(result.GetResultType(), Result::Type::Postcode, ());
    TEST(result.HasPoint(), ());

    auto const actual = result.GetFeatureCenter();
    TEST(base::AlmostEqualAbs(expected, actual, kMwmPointAccuracy), ());
  };

  test("BA6 7JP", MercatorBounds::FromLatLon(5.0, 4.0));
  test("BA6 7JP ", MercatorBounds::FromLatLon(5.0, 4.0));
  test("BA6 8JP", MercatorBounds::FromLatLon(5.0, 6.0));
  test("BA6 8JP ", MercatorBounds::FromLatLon(5.0, 6.0));
  // Search should return center of all inward codes for outward query.
  test("BA6", MercatorBounds::FromLatLon(5.0, 5.0));
  test("BA6 ", MercatorBounds::FromLatLon(5.0, 5.0));
}

UNIT_CLASS_TEST(PostcodePointsTest, SearchStreetWithPostcode)
{
  string const countryName = "Wonderland";

  Platform & platform = GetPlatform();
  auto const & writableDir = platform.WritableDir();
  string const testFile = "postcodes.csv";
  auto const postcodesRelativePath = base::JoinPath(writableDir, testFile);

  ScopedFile const osmScopedFile(
      testFile,
      "AA5 6KL, 4.0, 4.0\n"
      "BB7 8MN, 6.0, 6.0\n"
      "XX6 7KL, 4.0, 6.0\n"
      "YY8 9MN, 6.0, 4.0\n"
      // Some dummy postcodes to make postcode radius approximation not too big.
      "CC1 001, 5.0, 5.0\n"
      "CC1 002, 5.0, 5.0\n"
      "CC1 003, 5.0, 5.0\n"
      "CC1 004, 5.0, 5.0\n"
      "CC1 005, 5.0, 5.0\n"
      "CC1 006, 5.0, 5.0\n"
      "CC1 007, 5.0, 5.0\n"
      "CC1 008, 5.0, 5.0\n"
      "CC1 009, 5.0, 5.0\n"
      "CC1 010, 5.0, 5.0\n"
      "CC1 011, 5.0, 5.0\n"
      "CC1 012, 5.0, 5.0\n"
      "CC1 013, 5.0, 5.0\n"
      "CC1 014, 5.0, 5.0\n"
      "CC1 015, 5.0, 5.0\n"
      "CC1 016, 5.0, 5.0\n"
      "CC1 017, 5.0, 5.0\n"
      "CC1 018, 5.0, 5.0\n"
      "CC1 019, 5.0, 5.0\n"
      "CC1 020, 5.0, 5.0\n");

  auto const rect =
      m2::RectD(MercatorBounds::FromLatLon(3.99, 3.99), MercatorBounds::FromLatLon(6.01, 6.01));
  auto infoGetter = std::make_shared<storage::CountryInfoGetterForTesting>();
  infoGetter->AddCountry(storage::CountryDef(countryName, rect));

  TestStreet streetA(vector<m2::PointD>{MercatorBounds::FromLatLon(3.99, 3.99),
                                        MercatorBounds::FromLatLon(4.01, 4.01)},
                     "Garden street", "en");
  TestPOI houseA(MercatorBounds::FromLatLon(4.0, 4.0), "", "en");
  houseA.SetHouseNumber("1");
  houseA.SetStreetName(streetA.GetName("en"));
  TestStreet streetB(vector<m2::PointD>{MercatorBounds::FromLatLon(5.99, 5.99),
                                        MercatorBounds::FromLatLon(6.01, 6.01)},
                     "Garden street", "en");
  TestPOI houseB(MercatorBounds::FromLatLon(6.0, 6.0), "", "en");
  houseB.SetHouseNumber("1");
  houseB.SetStreetName(streetB.GetName("en"));
  TestStreet streetX(vector<m2::PointD>{MercatorBounds::FromLatLon(3.99, 5.99),
                                        MercatorBounds::FromLatLon(4.01, 6.01)},
                     "Main street", "en");
  TestStreet streetY(vector<m2::PointD>{MercatorBounds::FromLatLon(5.99, 3.99),
                                        MercatorBounds::FromLatLon(6.01, 4.01)},
                     "Main street", "en");

  auto const id = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.SetPostcodesData(postcodesRelativePath, infoGetter);
    builder.Add(streetA);
    builder.Add(houseA);
    builder.Add(streetB);
    builder.Add(houseB);
    builder.Add(streetX);
    builder.Add(streetY);
  });

  auto test = [&](string const & query, TestFeature const & bestResult) {
    auto request = MakeRequest(query);
    auto const & results = request->Results();

    TEST_GREATER_OR_EQUAL(results.size(), 1, ("Unexpected number of results."));
    TEST(ResultsMatch({results[0]}, {ExactMatch(id, bestResult)}), ());
    TEST(results[0].GetRankingInfo().m_allTokensUsed, ());
    for (size_t i = 1; i < results.size(); ++i)
      TEST(!results[i].GetRankingInfo().m_allTokensUsed, ());
  };
  SetViewport(rect);
  test("Garden street 1 AA5 6KL ", houseA);
  test("Garden street 1 BB7 8MN ", houseB);
  test("Main street XX6 7KL ", streetX);
  test("Main street YY8 9MN ", streetY);
}
}  // namespace
