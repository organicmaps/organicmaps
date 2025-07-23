#include "testing/testing.hpp"

#include "generator/affiliation.hpp"
#include "generator/feature_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "platform/platform.hpp"

#include "geometry/mercator.hpp"
#include "geometry/point2d.hpp"

#include "base/assert.hpp"
#include "base/file_name_utils.hpp"

#include <fstream>
#include <string>
#include <vector>

namespace
{
class AffiliationTests
{
public:
  static std::string const kOne;
  static std::string const kTwo;

  static constexpr m2::PointD kPointInsideOne1{3.0, 3.0};
  static constexpr m2::PointD kPointInsideOne2{4.0, 4.0};
  static constexpr m2::PointD kPointInsideTwo1{10.0, 3.0};
  static constexpr m2::PointD kPointInsideTwo2{11.0, 4.0};
  static constexpr m2::PointD kPointInsideOneAndTwo1{7.0, 4.0};
  static constexpr m2::PointD kPointInsideOneAndTwo2{9.0, 5.0};
  static constexpr m2::PointD kPointOnBorderOne{1.0, 6.0};
  static constexpr m2::PointD kPointOnBorderTwo{14.0, 6.0};
  static constexpr m2::PointD kPointInsideOneBoundingBox{1.0, 9.0};
  static constexpr m2::PointD kPointInsideTwoBoundingBox{14.0, 9.0};

  AffiliationTests()
  {
    classificator::Load();

    auto & platform = GetPlatform();
    m_testPath = base::JoinPath(platform.WritableDir(), "AffiliationTests");
    m_borderPath = base::JoinPath(m_testPath, "borders");
    CHECK(Platform::MkDirRecursively(m_borderPath), (m_borderPath));

    std::ofstream(base::JoinPath(m_borderPath, "One.poly")) <<
        R"(One
1
  5.0 0.0
  0.0 5.0
  5.0 10.0
  10.0 5.0
  5.0 0.0
END
END)";

    std::ofstream(base::JoinPath(m_borderPath, "Two.poly")) <<
        R"(Two
1
    10.0 0.0
    5.0 5.0
    10.0 10.0
    15.0 5.0
    10.0 0.0
END
END)";
  }

  ~AffiliationTests() { CHECK(Platform::RmDirRecursively(m_testPath), (m_testPath)); }

  std::string const & GetBorderPath() const { return m_testPath; }

  static feature::FeatureBuilder MakeLineFb(std::vector<m2::PointD> && geom)
  {
    feature::FeatureBuilder fb;
    fb.AssignPoints(std::move(geom));

    fb.AddType(classif().GetTypeByPath({"highway", "secondary"}));
    fb.SetLinear();
    return fb;
  }

private:
  std::string m_testPath;
  std::string m_borderPath;
};

// static
std::string const AffiliationTests::kOne = "One";
std::string const AffiliationTests::kTwo = "Two";

bool Test(std::vector<std::string> && res, std::set<std::string> const & answ)
{
  if (res.size() != answ.size())
    return false;

  std::set<std::string> r;
  std::move(std::begin(res), std::end(res), std::inserter(r, std::begin(r)));
  return r == answ;
}

void TestCountriesAffiliationInsideBorders(feature::AffiliationInterface const & affiliation)
{
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointInsideOne1), {AffiliationTests::kOne}), ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointInsideOne2), {AffiliationTests::kOne}), ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointOnBorderOne), {AffiliationTests::kOne}), ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointInsideTwo1), {AffiliationTests::kTwo}), ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointInsideTwo2), {AffiliationTests::kTwo}), ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointInsideOneAndTwo1),
            {AffiliationTests::kOne, AffiliationTests::kTwo}),
       ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointInsideOneAndTwo2),
            {AffiliationTests::kOne, AffiliationTests::kTwo}),
       ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointOnBorderTwo), {AffiliationTests::kTwo}), ());

  TEST(Test(affiliation.GetAffiliations(
                AffiliationTests::MakeLineFb({AffiliationTests::kPointInsideOne1, AffiliationTests::kPointInsideOne2})),
            {AffiliationTests::kOne}),
       ());
  TEST(Test(affiliation.GetAffiliations(
                AffiliationTests::MakeLineFb({AffiliationTests::kPointInsideTwo1, AffiliationTests::kPointInsideTwo2})),
            {AffiliationTests::kTwo}),
       ());
  TEST(Test(affiliation.GetAffiliations(
                AffiliationTests::MakeLineFb({AffiliationTests::kPointInsideOne1, AffiliationTests::kPointInsideTwo1})),
            {AffiliationTests::kOne, AffiliationTests::kTwo}),
       ());
}

template <typename T>
void TestCountriesFilesAffiliation(std::string const & borderPath)
{
  {
    T affiliation(borderPath, false /* haveBordersForWholeWorld */);

    TestCountriesAffiliationInsideBorders(affiliation);

    TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointInsideOneBoundingBox), {}), ());
    TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointInsideTwoBoundingBox), {}), ());
  }
  {
    T affiliation(borderPath, true /* haveBordersForWholeWorld */);

    TestCountriesAffiliationInsideBorders(affiliation);

    TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointInsideOneBoundingBox), {AffiliationTests::kOne}), ());
    TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointInsideTwoBoundingBox), {AffiliationTests::kTwo}), ());
  }
}

}  // namespace

UNIT_CLASS_TEST(AffiliationTests, SingleAffiliationTests)
{
  std::string const kName = "Name";
  feature::SingleAffiliation affiliation(kName);

  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointInsideOne1), {kName}), ());

  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointInsideOneAndTwo1), {kName}), ());

  TEST(Test(affiliation.GetAffiliations(
                AffiliationTests::MakeLineFb({AffiliationTests::kPointInsideOne1, AffiliationTests::kPointInsideTwo1})),
            {kName}),
       ());

  TEST(affiliation.HasCountryByName(kName), ());
  TEST(!affiliation.HasCountryByName("NoName"), ());
}

UNIT_CLASS_TEST(AffiliationTests, CountriesFilesAffiliationTests)
{
  TestCountriesFilesAffiliation<feature::CountriesFilesAffiliation>(AffiliationTests::GetBorderPath());
}

UNIT_CLASS_TEST(AffiliationTests, CountriesFilesIndexAffiliationTests)
{
  TestCountriesFilesAffiliation<feature::CountriesFilesIndexAffiliation>(AffiliationTests::GetBorderPath());
}

UNIT_TEST(Lithuania_Belarus_Border)
{
  using namespace borders;
  auto const bordersDir = base::JoinPath(GetPlatform().WritableDir(), BORDERS_DIR);

  // https://www.openstreetmap.org/node/3951697639 should belong to both countries.
  auto const point = mercator::FromLatLon({54.5443346, 25.6997363});

  for (auto const country : {"Lithuania_East", "Belarus_Hrodna Region"})
  {
    std::vector<m2::RegionD> regions;
    LoadBorders(bordersDir + country + BORDERS_EXTENSION, regions);
    TEST_EQUAL(regions.size(), 1, ());

    bool found = false;
    for (auto const eps : {1.0E-5, 5.0E-5, 1.0E-4})
    {
      if (regions[0].Contains(point, CountryPolygons::ContainsCompareFn(eps)))
      {
        LOG(LINFO, (eps, country));
        TEST_LESS_OR_EQUAL(eps, CountryPolygons::GetContainsEpsilon(), ());
        found = true;
        break;
      }
    }
    TEST(found, (country));
  }
}
