#include "search/search_tests_support/test_feature.hpp"

#include "generator/feature_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "base/assert.hpp"

#include "std/sstream.hpp"

namespace search
{
namespace tests_support
{
TestFeature::TestFeature(string const & name, string const & lang) : m_name(name), m_lang(lang) {}

TestFeature::TestFeature(m2::PointD const & center, string const & name, string const & lang)
  : m_center(center), m_name(name), m_lang(lang)
{
}

void TestFeature::Serialize(FeatureBuilder1 & fb) const
{
  fb.SetCenter(m_center);
  CHECK(fb.AddName(m_lang, m_name), ("Can't set feature name:", m_name, "(", m_lang, ")"));
}

bool TestFeature::Matches(FeatureType const & feature) const
{
  uint8_t const langIndex = StringUtf8Multilang::GetLangIndex(m_lang);
  string name;
  return feature.GetName(langIndex, name) && m_name == name;
}

// TestPOI -----------------------------------------------------------------------------------------
TestPOI::TestPOI(m2::PointD const & center, string const & name, string const & lang)
  : TestFeature(center, name, lang)
{
}

void TestPOI::Serialize(FeatureBuilder1 & fb) const
{
  TestFeature::Serialize(fb);
  auto const & classificator = classif();
  fb.SetType(classificator.GetTypeByPath({"railway", "station"}));
}

string TestPOI::ToString() const
{
  ostringstream os;
  os << "TestPOI [" << m_name << ", " << m_lang << ", " << DebugPrint(m_center) << "]";
  return os.str();
}

// TestCity ----------------------------------------------------------------------------------------
TestCity::TestCity(m2::PointD const & center, string const & name, string const & lang,
                   uint8_t rank)
  : TestFeature(center, name, lang), m_rank(rank)
{
}

void TestCity::Serialize(FeatureBuilder1 & fb) const
{
  TestFeature::Serialize(fb);
  auto const & classificator = classif();
  fb.SetType(classificator.GetTypeByPath({"place", "city"}));
  fb.SetRank(m_rank);
}

string TestCity::ToString() const
{
  ostringstream os;
  os << "TestCity [" << m_name << ", " << m_lang << ", " << DebugPrint(m_center) << "]";
  return os.str();
}

// TestStreet --------------------------------------------------------------------------------------
TestStreet::TestStreet(vector<m2::PointD> const & points, string const & name, string const & lang)
  : TestFeature(name, lang), m_points(points)
{
}

void TestStreet::Serialize(FeatureBuilder1 & fb) const
{
  CHECK(fb.AddName(m_lang, m_name), ("Can't set feature name:", m_name, "(", m_lang, ")"));
  if (m_lang != "default")
    CHECK(fb.AddName("default", m_name), ("Can't set feature name:", m_name, "( default )"));

  auto const & classificator = classif();
  fb.SetType(classificator.GetTypeByPath({"highway", "living_street"}));

  for (auto const & point : m_points)
    fb.AddPoint(point);
  fb.SetLinear(false /* reverseGeometry */);
}

string TestStreet::ToString() const
{
  ostringstream os;
  os << "TestStreet [" << m_name << ", " << m_lang << ", " << ::DebugPrint(m_points) << "]";
  return os.str();
}

// TestBuilding ------------------------------------------------------------------------------------
TestBuilding::TestBuilding(m2::PointD const & center, string const & name,
                           string const & houseNumber, string const & lang)
  : TestFeature(center, name, lang), m_houseNumber(houseNumber)
{
}

TestBuilding::TestBuilding(m2::PointD const & center, string const & name,
                           string const & houseNumber, TestStreet const & street,
                           string const & lang)
  : TestFeature(center, name, lang)
  , m_houseNumber(houseNumber)
  , m_streetName(street.GetName())
{
}

void TestBuilding::Serialize(FeatureBuilder1 & fb) const
{
  TestFeature::Serialize(fb);
  fb.AddHouseNumber(m_houseNumber);
  if (!m_streetName.empty())
    fb.AddStreet(m_streetName);

  auto const & classificator = classif();
  fb.SetType(classificator.GetTypeByPath({"building"}));

  fb.PreSerialize();
}

bool TestBuilding::Matches(FeatureType const & feature) const
{
  auto const & checker = ftypes::IsBuildingChecker::Instance();
  if (!checker(feature))
    return false;
  return TestFeature::Matches(feature) && m_houseNumber == feature.GetHouseNumber();
}

string TestBuilding::ToString() const
{
  ostringstream os;
  os << "TestBuilding [" << m_name << ", " << m_houseNumber << ", " << m_lang << ", "
     << DebugPrint(m_center) << "]";
  return os.str();
}

string DebugPrint(TestFeature const & feature) { return feature.ToString(); }
}  // namespace tests_support
}  // namespace search
