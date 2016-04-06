#include "search/search_tests_support/test_feature.hpp"

#include "generator/feature_builder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "coding/multilang_utf8_string.hpp"

#include "base/assert.hpp"

#include "std/atomic.hpp"
#include "std/sstream.hpp"

namespace search
{
namespace tests_support
{
namespace
{
uint64_t GenUniqueId()
{
  static atomic<uint64_t> id;
  return id.fetch_add(1);
}
}  // namespace

// TestFeature -------------------------------------------------------------------------------------
TestFeature::TestFeature(string const & name, string const & lang)
  : m_id(GenUniqueId()), m_center(0, 0), m_hasCenter(false), m_name(name), m_lang(lang)
{
}

TestFeature::TestFeature(m2::PointD const & center, string const & name, string const & lang)
  : m_id(GenUniqueId()), m_center(center), m_hasCenter(true), m_name(name), m_lang(lang)
{
}

bool TestFeature::Matches(FeatureType const & feature) const
{
  istringstream is(feature.GetMetadata().Get(feature::Metadata::FMD_TEST_ID));
  uint64_t id;
  is >> id;
  return id == m_id;
}

void TestFeature::Serialize(FeatureBuilder1 & fb) const
{
  fb.SetTestId(m_id);
  if (m_hasCenter)
    fb.SetCenter(m_center);
  if (!m_name.empty())
    CHECK(fb.AddName(m_lang, m_name), ("Can't set feature name:", m_name, "(", m_lang, ")"));
}

// TestCountry -------------------------------------------------------------------------------------
TestCountry::TestCountry(m2::PointD const & center, string const & name, string const & lang)
  : TestFeature(center, name, lang)
{
}

void TestCountry::Serialize(FeatureBuilder1 & fb) const
{
  TestFeature::Serialize(fb);
  auto const & classificator = classif();
  fb.SetType(classificator.GetTypeByPath({"place", "country"}));

  // Localities should have default name too.
  fb.AddName("default", m_name);
}

string TestCountry::ToString() const
{
  ostringstream os;
  os << "TestCountry [" << m_name << ", " << m_lang << ", " << DebugPrint(m_center) << "]";
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

// TestVillage
// ----------------------------------------------------------------------------------------
TestVillage::TestVillage(m2::PointD const & center, string const & name, string const & lang,
                         uint8_t rank)
  : TestFeature(center, name, lang), m_rank(rank)
{
}

void TestVillage::Serialize(FeatureBuilder1 & fb) const
{
  TestFeature::Serialize(fb);
  auto const & classificator = classif();
  fb.SetType(classificator.GetTypeByPath({"place", "village"}));
  fb.SetRank(m_rank);
}

string TestVillage::ToString() const
{
  ostringstream os;
  os << "TestVillage [" << m_name << ", " << m_lang << ", " << DebugPrint(m_center) << "]";
  return os.str();
}

// TestStreet --------------------------------------------------------------------------------------
TestStreet::TestStreet(vector<m2::PointD> const & points, string const & name, string const & lang)
  : TestFeature(name, lang), m_points(points)
{
}

void TestStreet::Serialize(FeatureBuilder1 & fb) const
{
  fb.SetTestId(m_id);
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

// TestPOI -----------------------------------------------------------------------------------------
TestPOI::TestPOI(m2::PointD const & center, string const & name, string const & lang)
  : TestFeature(center, name, lang)
{
  m_types = {{"railway", "station"}};
}

void TestPOI::Serialize(FeatureBuilder1 & fb) const
{
  TestFeature::Serialize(fb);
  auto const & classificator = classif();

  for (auto const & path : m_types)
    fb.SetType(classificator.GetTypeByPath(path));

  if (!m_houseNumber.empty())
    fb.AddHouseNumber(m_houseNumber);
  if (!m_streetName.empty())
    fb.AddStreet(m_streetName);
}

string TestPOI::ToString() const
{
  ostringstream os;
  os << "TestPOI [" << m_name << ", " << m_lang << ", " << DebugPrint(m_center);
  if (!m_houseNumber.empty())
    os << ", " << m_houseNumber;
  if (!m_streetName.empty())
    os << ", " << m_streetName;
  os << "]";
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

TestBuilding::TestBuilding(vector<m2::PointD> const & boundary, string const & name,
                           string const & houseNumber, TestStreet const & street,
                           string const & lang)
  : TestFeature(name, lang)
  , m_boundary(boundary)
  , m_houseNumber(houseNumber)
  , m_streetName(street.GetName())
{
  ASSERT(!m_boundary.empty(), ());
}

void TestBuilding::Serialize(FeatureBuilder1 & fb) const
{
  TestFeature::Serialize(fb);
  if (!m_hasCenter)
  {
    for (auto const & point : m_boundary)
      fb.AddPoint(point);
    fb.SetArea();
  }
  fb.AddHouseNumber(m_houseNumber);
  if (!m_streetName.empty())
    fb.AddStreet(m_streetName);

  auto const & classificator = classif();
  fb.SetType(classificator.GetTypeByPath({"building"}));
}

string TestBuilding::ToString() const
{
  ostringstream os;
  os << "TestBuilding [" << m_name << ", " << m_houseNumber << ", " << m_lang << ", "
     << DebugPrint(m_center) << "]";
  return os.str();
}

// TestPark ----------------------------------------------------------------------------------------
TestPark::TestPark(vector<m2::PointD> const & boundary, string const & name, string const & lang)
  : TestFeature(name, lang), m_boundary(boundary)
{
}

void TestPark::Serialize(FeatureBuilder1 & fb) const
{
  TestFeature::Serialize(fb);
  for (auto const & point : m_boundary)
    fb.AddPoint(point);
  fb.SetArea();

  auto const & classificator = classif();
  fb.SetType(classificator.GetTypeByPath({"leisure", "park"}));
}

string TestPark::ToString() const
{
  ostringstream os;
  os << "TestPark [" << m_name << ", " << m_lang << "]";
  return os.str();
}

string DebugPrint(TestFeature const & feature) { return feature.ToString(); }
}  // namespace tests_support
}  // namespace search
