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

  ScopedFile const osmScopedFile(
      testFile,
      "aa11 0, dummy, 1000, 1000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "aa11 1, dummy, 2000, 2000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "aa11 2, dummy, 3000, 3000, dummy, dummy, dummy, dummy, dummy, dummy\n");

  auto infoGetter = std::make_shared<storage::CountryInfoGetterForTesting>();
  infoGetter->AddCountry(
      storage::CountryDef(countryName, m2::RectD(MercatorBounds::UKCoordsToXY(999, 999),
                                                 MercatorBounds::UKCoordsToXY(30001, 30001))));

  auto const id = BuildCountry(countryName, [&](TestMwmBuilder & builder) {
    builder.SetPostcodesData(postcodesRelativePath, infoGetter);
  });

  auto handle = m_dataSource.GetMwmHandleById(id);
  auto value = handle.GetValue<MwmValue>();
  CHECK(value, ());
  TEST(value->m_cont.IsExist(POSTCODE_POINTS_FILE_TAG), ());

  PostcodePoints p(*value);
  {
    vector<m2::PointD> points;
    p.Get(NormalizeAndSimplifyString("aa11 0"), points);
    TEST_EQUAL(points.size(), 1, ());
    TEST(base::AlmostEqualAbs(points[0], MercatorBounds::UKCoordsToXY(1000, 1000),
                              kMwmPointAccuracy),
         ());
  }
  {
    vector<m2::PointD> points;
    p.Get(NormalizeAndSimplifyString("aa11 1"), points);
    TEST_EQUAL(points.size(), 1, ());
    TEST(base::AlmostEqualAbs(points[0], MercatorBounds::UKCoordsToXY(2000, 2000),
                              kMwmPointAccuracy),
         ());
  }
  {
    vector<m2::PointD> points;
    p.Get(NormalizeAndSimplifyString("aa11 2"), points);
    TEST_EQUAL(points.size(), 1, ());
    TEST(base::AlmostEqualAbs(points[0], MercatorBounds::UKCoordsToXY(3000, 3000),
                              kMwmPointAccuracy),
         ());
  }
  {
    vector<m2::PointD> points;
    p.Get(NormalizeAndSimplifyString("aa11"), points);
    TEST_EQUAL(points.size(), 3, ());
    sort(points.begin(), points.end());
    TEST(base::AlmostEqualAbs(points[0], MercatorBounds::UKCoordsToXY(1000, 1000),
                              kMwmPointAccuracy),
         ());
    TEST(base::AlmostEqualAbs(points[1], MercatorBounds::UKCoordsToXY(2000, 2000),
                              kMwmPointAccuracy),
         ());
    TEST(base::AlmostEqualAbs(points[2], MercatorBounds::UKCoordsToXY(3000, 3000),
                              kMwmPointAccuracy),
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

  ScopedFile const osmScopedFile(
      testFile,
      "BA6 7JP, dummy, 4000, 4000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "BA6 8JP, dummy, 6000, 6000, dummy, dummy, dummy, dummy, dummy, dummy\n");

  auto infoGetter = std::make_shared<storage::CountryInfoGetterForTesting>();
  infoGetter->AddCountry(
      storage::CountryDef(countryName, m2::RectD(MercatorBounds::UKCoordsToXY(3000, 3000),
                                                 MercatorBounds::UKCoordsToXY(7000, 7000))));

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

  test("BA6 7JP", MercatorBounds::UKCoordsToXY(4000, 4000));
  test("BA6 7JP ", MercatorBounds::UKCoordsToXY(4000, 4000));
  test("BA6 8JP", MercatorBounds::UKCoordsToXY(6000, 6000));
  test("BA6 8JP ", MercatorBounds::UKCoordsToXY(6000, 6000));
  // Search should return center of all inward codes for outward query.
  test("BA6", MercatorBounds::UKCoordsToXY(5000, 5000));
  test("BA6 ", MercatorBounds::UKCoordsToXY(5000, 5000));
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
      "AA5 6KL, dummy, 4000, 4000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "BB7 8MN, dummy, 6000, 6000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "XX6 7KL, dummy, 4000, 6000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "YY8 9MN, dummy, 6000, 4000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      // Some dummy postcodes to make postcode radius approximation not too big.
      "CC1 001, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 002, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 003, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 004, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 005, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 006, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 007, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 008, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 009, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 010, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 011, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 012, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 013, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 014, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 015, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 016, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 017, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 018, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 019, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n"
      "CC1 020, dummy, 5000, 5000, dummy, dummy, dummy, dummy, dummy, dummy\n");

  auto const rect =
      m2::RectD(MercatorBounds::UKCoordsToXY(3990, 3990), MercatorBounds::UKCoordsToXY(6010, 6010));
  auto infoGetter = std::make_shared<storage::CountryInfoGetterForTesting>();
  infoGetter->AddCountry(storage::CountryDef(countryName, rect));

  TestStreet streetA(vector<m2::PointD>{MercatorBounds::UKCoordsToXY(3990, 3990),
                                        MercatorBounds::UKCoordsToXY(4010, 4010)},
                     "Garden street", "en");
  TestPOI houseA(MercatorBounds::UKCoordsToXY(4000, 4000), "", "en");
  houseA.SetHouseNumber("1");
  houseA.SetStreetName(streetA.GetName("en"));
  TestStreet streetB(vector<m2::PointD>{MercatorBounds::UKCoordsToXY(5990, 5990),
                                        MercatorBounds::UKCoordsToXY(6010, 6010)},
                     "Garden street", "en");
  TestPOI houseB(MercatorBounds::UKCoordsToXY(6000, 6000), "", "en");
  houseB.SetHouseNumber("1");
  houseB.SetStreetName(streetB.GetName("en"));
  TestStreet streetX(vector<m2::PointD>{MercatorBounds::UKCoordsToXY(3990, 5990),
                                        MercatorBounds::UKCoordsToXY(4010, 6010)},
                     "Main street", "en");
  TestStreet streetY(vector<m2::PointD>{MercatorBounds::UKCoordsToXY(5990, 3990),
                                        MercatorBounds::UKCoordsToXY(6010, 4010)},
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
