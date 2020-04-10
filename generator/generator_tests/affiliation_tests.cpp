#include "testing/testing.hpp"

#include "generator/affiliation.hpp"
#include "generator/feature_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

#include "platform/platform.hpp"

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
  static const std::string kOne;
  static const std::string kTwo;

  static constexpr m2::PointD kPointOne1{3.0, 3.0};
  static constexpr m2::PointD kPointOne2{4.0, 4.0};
  static constexpr m2::PointD kPointTwo1{10.0, 3.0};
  static constexpr m2::PointD kPointTwo2{11.0, 4.0};
  static constexpr m2::PointD kPointOneTwo1{7.0, 4.0};
  static constexpr m2::PointD kPointOneTwo2{9.0, 5.0};
  static constexpr m2::PointD kPointOneBb{1.0, 9.0};
  static constexpr m2::PointD kPointTwoBb{14.0, 9.0};

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

  static feature::FeatureBuilder MakeLineFb(std::vector<m2::PointD> const & geom)
  {
    feature::FeatureBuilder fb;
    for (auto const & p : geom)
      fb.AddPoint(p);

    fb.AddType(classif().GetTypeByPath({"highway", "secondary"}));
    fb.SetLinear();
    return fb;
  }

private:
  std::string m_testPath;
  std::string m_borderPath;
};

// static
const std::string AffiliationTests::kOne = "One";
const std::string AffiliationTests::kTwo = "Two";

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
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointOne1), {AffiliationTests::kOne}),
       ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointOne2), {AffiliationTests::kOne}),
       ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointTwo1), {AffiliationTests::kTwo}),
       ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointTwo2), {AffiliationTests::kTwo}),
       ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointOneTwo1),
            {AffiliationTests::kOne, AffiliationTests::kTwo}),
       ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointOneTwo2),
            {AffiliationTests::kOne, AffiliationTests::kTwo}),
       ());

  TEST(Test(affiliation.GetAffiliations(AffiliationTests::MakeLineFb(
                {AffiliationTests::kPointOne1, AffiliationTests::kPointOne2})),
            {AffiliationTests::kOne}),
       ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::MakeLineFb(
                {AffiliationTests::kPointTwo1, AffiliationTests::kPointTwo2})),
            {AffiliationTests::kTwo}),
       ());
  TEST(Test(affiliation.GetAffiliations(AffiliationTests::MakeLineFb(
                {AffiliationTests::kPointOne1, AffiliationTests::kPointTwo1})),
            {AffiliationTests::kOne, AffiliationTests::kTwo}),
       ());
}

template <typename T>
void TestCountriesFilesAffiliation(std::string const & borderPath)
{
  {
    T affiliation(borderPath, false /* haveBordersForWholeWorld */);

    TestCountriesAffiliationInsideBorders(affiliation);

    TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointOneBb), {}), ());
    TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointTwoBb), {}), ());
  }
  {
    T affiliation(borderPath, true /* haveBordersForWholeWorld */);

    TestCountriesAffiliationInsideBorders(affiliation);

    TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointOneBb), {AffiliationTests::kOne}),
         ());
    TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointTwoBb), {AffiliationTests::kTwo}),
         ());
  }
}

UNIT_CLASS_TEST(AffiliationTests, SingleAffiliationTests)
{
  std::string const kName = "Name";
  feature::SingleAffiliation affiliation(kName);

  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointOne1), {kName}), ());

  TEST(Test(affiliation.GetAffiliations(AffiliationTests::kPointOneTwo1), {kName}), ());

  TEST(Test(affiliation.GetAffiliations(AffiliationTests::MakeLineFb(
                {AffiliationTests::kPointOne1, AffiliationTests::kPointTwo1})),
            {kName}),
       ());

  TEST(affiliation.HasCountryByName(kName), ());
  TEST(!affiliation.HasCountryByName("NoName"), ());
}

UNIT_CLASS_TEST(AffiliationTests, CountriesFilesAffiliationTests)
{
  TestCountriesFilesAffiliation<feature::CountriesFilesAffiliation>(
      AffiliationTests::GetBorderPath());
}

UNIT_CLASS_TEST(AffiliationTests, CountriesFilesIndexAffiliationTests)
{
  TestCountriesFilesAffiliation<feature::CountriesFilesIndexAffiliation>(
      AffiliationTests::GetBorderPath());
}
}  // namespace
