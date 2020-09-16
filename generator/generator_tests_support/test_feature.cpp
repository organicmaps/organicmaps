#include "generator/generator_tests_support/test_feature.hpp"

#include "generator/feature_builder.hpp"

#include "editor/osm_editor.hpp"

#include "indexer/classificator.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/mwm_set.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <atomic>
#include <sstream>

using namespace std;
using namespace feature;

namespace generator
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
TestFeature::TestFeature() : m_id(GenUniqueId()), m_center(0, 0), m_type(Type::Unknown) { Init(); }

TestFeature::TestFeature(string const & name, string const & lang)
  : m_id(GenUniqueId()), m_center(0, 0), m_type(Type::Unknown)
{
  m_names.AddString(lang, name);

  // Names used for search depend on locale. Fill default name cause we need to run tests with
  // different locales. If you do not need default name to be filled use
  // TestFeature::TestFeature(StringUtf8Multilang const & name).
  m_names.AddString("default", name);
  Init();
}

TestFeature::TestFeature(StringUtf8Multilang const & name)
  : m_id(GenUniqueId()), m_center(0, 0), m_type(Type::Unknown), m_names(name)
{
}

TestFeature::TestFeature(m2::PointD const & center, string const & name, string const & lang)
  : m_id(GenUniqueId()), m_center(center), m_type(Type::Point)
{
  m_names.AddString(lang, name);
  m_names.AddString("default", name);
  Init();
}

TestFeature::TestFeature(m2::PointD const & center, StringUtf8Multilang const & name)
  : m_id(GenUniqueId()), m_center(center), m_type(Type::Point), m_names(name)
{
  Init();
}

TestFeature::TestFeature(vector<m2::PointD> const & boundary, string const & name,
                         string const & lang)
  : m_id(GenUniqueId()), m_boundary(boundary), m_type(Type::Area)
{
  m_names.AddString(lang, name);
  m_names.AddString("default", name);
  ASSERT(!m_boundary.empty(), ());
  Init();
}

void TestFeature::Init()
{
  m_metadata.Set(Metadata::FMD_TEST_ID, strings::to_string(m_id));
}

bool TestFeature::Matches(FeatureType & feature) const
{
  istringstream is(feature.GetMetadata(Metadata::FMD_TEST_ID));
  uint64_t id;
  is >> id;
  return id == m_id;
}

void TestFeature::Serialize(FeatureBuilder & fb) const
{
  using feature::Metadata;
  // Metadata::EType::FMD_CUISINE is the first enum value.
  size_t i = static_cast<size_t>(Metadata::EType::FMD_CUISINE);
  size_t const count = static_cast<size_t>(Metadata::EType::FMD_COUNT);
  for (; i < count; ++i)
  {
    auto const type = static_cast<Metadata::EType>(i);
    if (m_metadata.Has(type))
    {
      auto const value = m_metadata.Get(type);
      fb.GetMetadata().Set(type, value);
    }
  }

  switch (m_type)
  {
  case Type::Point:
  {
    fb.SetCenter(m_center);
    break;
  }
  case Type::Area:
  {
    ASSERT(!m_boundary.empty(), ());
    for (auto const & p : m_boundary)
      fb.AddPoint(p);
    fb.SetArea();
    break;
  }
  case Type::Unknown: break;
  }

  m_names.ForEach([&](int8_t langCode, string const & name) {
    if (!name.empty())
    {
      auto const lang = StringUtf8Multilang::GetLangByCode(langCode);
      CHECK(fb.AddName(lang, name), ("Can't set feature name:", name, "(", lang, ")"));
    }
  });

  if (!m_postcode.empty())
  {
    fb.GetParams().AddPostcode(m_postcode);
    fb.GetMetadata().Set(Metadata::FMD_POSTCODE, m_postcode);
  }
}

// TestCountry -------------------------------------------------------------------------------------
TestCountry::TestCountry(m2::PointD const & center, string const & name, string const & lang)
  : TestFeature(center, name, lang)
{
}

void TestCountry::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);
  auto const & classificator = classif();
  fb.AddType(classificator.GetTypeByPath({"place", "country"}));
}

string TestCountry::ToDebugString() const
{
  ostringstream os;
  os << "TestCountry [" << DebugPrint(m_names) << ", " << DebugPrint(m_center) << "]";
  return os.str();
}

// TestState -------------------------------------------------------------------------------------
TestState::TestState(m2::PointD const & center, string const & name, string const & lang)
  : TestFeature(center, name, lang)
{
}

void TestState::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);
  auto const & classificator = classif();
  fb.AddType(classificator.GetTypeByPath({"place", "state"}));
}

string TestState::ToDebugString() const
{
  ostringstream os;
  os << "TestState [" << DebugPrint(m_names) << ", " << DebugPrint(m_center) << "]";
  return os.str();
}

// TestCity ----------------------------------------------------------------------------------------
TestCity::TestCity(m2::PointD const & center, string const & name, string const & lang,
                   uint8_t rank)
  : TestFeature(center, name, lang), m_rank(rank)
{
}

TestCity::TestCity(m2::PointD const & center, StringUtf8Multilang const & name, uint8_t rank)
  : TestFeature(center, name), m_rank(rank)
{
}

TestCity::TestCity(vector<m2::PointD> const & boundary, string const & name, string const & lang,
                   uint8_t rank)
  : TestFeature(boundary, name, lang), m_rank(rank)
{
}

void TestCity::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);
  auto const & classificator = classif();
  fb.AddType(classificator.GetTypeByPath({"place", "city"}));
  fb.SetRank(m_rank);
}

string TestCity::ToDebugString() const
{
  ostringstream os;
  os << "TestCity [" << DebugPrint(m_names) << ", " << DebugPrint(m_center) << "]";
  return os.str();
}

// TestVillage
// ----------------------------------------------------------------------------------------
TestVillage::TestVillage(m2::PointD const & center, string const & name, string const & lang,
                         uint8_t rank)
  : TestFeature(center, name, lang), m_rank(rank)
{
}

void TestVillage::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);
  auto const & classificator = classif();
  fb.AddType(classificator.GetTypeByPath({"place", "village"}));
  fb.SetRank(m_rank);
}

string TestVillage::ToDebugString() const
{
  ostringstream os;
  os << "TestVillage [" << DebugPrint(m_names) << ", " << DebugPrint(m_center) << "]";
  return os.str();
}

// TestStreet --------------------------------------------------------------------------------------
TestStreet::TestStreet(vector<m2::PointD> const & points, string const & name, string const & lang)
  : TestFeature(name, lang), m_points(points), m_highwayType("living_street")
{
}

TestStreet::TestStreet(vector<m2::PointD> const & points, StringUtf8Multilang const & name)
  : TestFeature(name), m_points(points), m_highwayType("living_street")
{
}

void TestStreet::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);

  auto const & classificator = classif();
  fb.SetType(classificator.GetTypeByPath({"highway", m_highwayType}));

  fb.GetParams().ref = m_roadNumber;

  for (auto const & point : m_points)
    fb.AddPoint(point);
  fb.SetLinear(false /* reverseGeometry */);
}

string TestStreet::ToDebugString() const
{
  ostringstream os;
  os << "TestStreet [" << DebugPrint(m_names) << ", " << ::DebugPrint(m_points) << "]";
  return os.str();
}

// TestSquare --------------------------------------------------------------------------------------
TestSquare::TestSquare(m2::RectD const & rect, string const & name, string const & lang)
  : TestFeature(name, lang), m_rect(rect)
{
}

void TestSquare::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);

  auto const & classificator = classif();
  fb.SetType(classificator.GetTypeByPath({"place", "square"}));

  fb.AddPoint(m_rect.LeftBottom());
  fb.AddPoint(m_rect.RightBottom());
  fb.AddPoint(m_rect.RightTop());
  fb.AddPoint(m_rect.LeftTop());
  fb.AddPoint(m_rect.LeftBottom());
  fb.SetArea();
}

string TestSquare::ToDebugString() const
{
  ostringstream os;
  os << "TestSquare [" << DebugPrint(m_names) << ", " << m_rect << "]";
  return os.str();
}

// TestPOI -----------------------------------------------------------------------------------------
TestPOI::TestPOI(m2::PointD const & center, string const & name, string const & lang)
  : TestFeature(center, name, lang)
{
  m_types = {{"railway", "station"}};
}

// static
pair<TestPOI, FeatureID> TestPOI::AddWithEditor(osm::Editor & editor, MwmSet::MwmId const & mwmId,
                                                string const & enName, m2::PointD const & pt)
{
  TestPOI poi(pt, enName, "en");

  osm::EditableMapObject emo;
  editor.CreatePoint(classif().GetTypeByPath({"shop", "bakery"}), pt, mwmId, emo);

  StringUtf8Multilang names;
  names.AddString(StringUtf8Multilang::GetLangIndex("en"), enName);
  emo.SetName(names);
  emo.SetTestId(poi.GetId());

  editor.SaveEditedFeature(emo);
  return {poi, emo.GetID()};
}

void TestPOI::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);
  auto const & classificator = classif();

  for (auto const & path : m_types)
    fb.AddType(classificator.GetTypeByPath(path));

  auto & params = fb.GetParams();
  if (!m_houseNumber.empty())
    params.AddHouseNumber(m_houseNumber);
  if (!m_streetName.empty())
    params.AddStreet(m_streetName);
}

string TestPOI::ToDebugString() const
{
  ostringstream os;
  os << "TestPOI [" << DebugPrint(m_names) << ", " << DebugPrint(m_center);
  if (!m_houseNumber.empty())
    os << ", " << m_houseNumber;
  if (!m_streetName.empty())
    os << ", " << m_streetName;
  os << "]";
  return os.str();
}

TypesHolder TestPOI::GetTypes() const
{
  TypesHolder types;
  for (auto const path : m_types)
    types.Add(classif().GetTypeByPath(path));
  return types;
}

// TestMultilingualPOI -----------------------------------------------------------------------------
TestMultilingualPOI::TestMultilingualPOI(m2::PointD const & center, string const & defaultName,
                                         map<string, string> const & multilingualNames)
  : TestPOI(center, defaultName, "default"), m_multilingualNames(multilingualNames)
{
}

void TestMultilingualPOI::Serialize(FeatureBuilder & fb) const
{
  TestPOI::Serialize(fb);

  for (auto const & kv : m_multilingualNames)
  {
    CHECK(fb.AddName(kv.first, kv.second),
          ("Can't set feature name:", kv.second, "(", kv.first, ")"));
  }
}

string TestMultilingualPOI::ToDebugString() const
{
  ostringstream os;
  os << "TestPOI [" << DebugPrint(m_names) << ", ";
  for (auto const & kv : m_multilingualNames)
    os << "( " << kv.second << ", " << kv.first << "), ";
  os << DebugPrint(m_center);
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
                           string const & houseNumber, string const & street, string const & lang)
  : TestFeature(center, name, lang), m_houseNumber(houseNumber), m_streetName(street)
{
}

TestBuilding::TestBuilding(vector<m2::PointD> const & boundary, string const & name,
                           string const & houseNumber, string const & street, string const & lang)
  : TestFeature(boundary, name, lang)
  , m_boundary(boundary)
  , m_houseNumber(houseNumber)
  , m_streetName(street)
{
}

void TestBuilding::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);

  auto & params = fb.GetParams();
  params.AddHouseNumber(m_houseNumber);
  if (!m_streetName.empty())
    params.AddStreet(m_streetName);

  auto const & classificator = classif();
  fb.AddType(classificator.GetTypeByPath({"building"}));
  for (auto const & type : m_types)
    fb.AddType(classificator.GetTypeByPath(type));
}

string TestBuilding::ToDebugString() const
{
  ostringstream os;
  os << "TestBuilding [" << DebugPrint(m_names) << ", " << m_houseNumber << ", "
     << DebugPrint(m_center) << "]";
  return os.str();
}

// TestPark ----------------------------------------------------------------------------------------
TestPark::TestPark(vector<m2::PointD> const & boundary, string const & name, string const & lang)
  : TestFeature(name, lang), m_boundary(boundary)
{
}

void TestPark::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);
  for (auto const & point : m_boundary)
    fb.AddPoint(point);
  fb.SetArea();

  auto const & classificator = classif();
  fb.AddType(classificator.GetTypeByPath({"leisure", "park"}));
}

string TestPark::ToDebugString() const
{
  ostringstream os;
  os << "TestPark [" << DebugPrint(m_names) << ", "
     << "]";
  return os.str();
}

// TestRoad ----------------------------------------------------------------------------------------
TestRoad::TestRoad(vector<m2::PointD> const & points, string const & name, string const & lang)
  : TestFeature(name, lang), m_points(points)
{
}

void TestRoad::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);

  auto const & classificator = classif();
  fb.AddType(classificator.GetTypeByPath({"highway", "road"}));

  for (auto const & point : m_points)
    fb.AddPoint(point);
  fb.SetLinear(false /* reverseGeometry */);
}

string TestRoad::ToDebugString() const
{
  ostringstream os;
  os << "TestRoad [" << DebugPrint(m_names) << "]";
  return os.str();
}

// Functions ---------------------------------------------------------------------------------------
string DebugPrint(TestFeature const & feature) { return feature.ToDebugString(); }
}  // namespace tests_support
}  // namespace generator
