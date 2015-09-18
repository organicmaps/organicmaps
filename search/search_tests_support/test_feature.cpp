#include "search/search_tests_support/test_feature.hpp"

#include "generator/feature_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "std/sstream.hpp"

#include "base/assert.hpp"

namespace search
{
namespace tests_support
{
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
  if (!feature.GetName(langIndex, name) || m_name != name)
    return false;
  return true;
}

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

string DebugPrint(TestFeature const & feature) { return feature.ToString(); }
}  // namespace tests_support
}  // namespace search
