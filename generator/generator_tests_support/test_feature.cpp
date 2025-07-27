#include "generator/generator_tests_support/test_feature.hpp"

#include "generator/feature_builder.hpp"

#include "editor/osm_editor.hpp"

#include "indexer/classificator.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/feature.hpp"
#include "indexer/feature_decl.hpp"
#include "indexer/feature_meta.hpp"
#include "indexer/mwm_set.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <atomic>
#include <sstream>

namespace generator
{
namespace tests_support
{
using namespace feature;
using std::ostringstream, std::string, std::string_view, std::vector;

namespace
{
uint64_t GenUniqueId()
{
  static std::atomic<uint64_t> id;
  return id.fetch_add(1);
}

vector<m2::PointD> MakePoly(m2::RectD const & rect)
{
  return {rect.LeftBottom(), rect.RightBottom(), rect.RightTop(), rect.LeftTop(), rect.LeftBottom()};
}

StringUtf8Multilang MakeName(string const & name, string const & lang)
{
  StringUtf8Multilang res;
  res.AddString(lang, name);

  // Names used for search depend on locale. Fill default name because we need to run tests with
  // different locales. If you do not need default name to be filled use
  // TestFeature::TestFeature(StringUtf8Multilang const & name).
  res.AddString("default", name);
  return res;
}
}  // namespace

// TestFeature -------------------------------------------------------------------------------------
TestFeature::TestFeature() : m_id(GenUniqueId()), m_center(0, 0), m_type(Type::Unknown)
{
  Init();
}

TestFeature::TestFeature(StringUtf8Multilang name)
  : m_id(GenUniqueId())
  , m_center(0, 0)
  , m_type(Type::Unknown)
  , m_names(std::move(name))
{
  Init();
}

TestFeature::TestFeature(m2::PointD const & center, StringUtf8Multilang name)
  : m_id(GenUniqueId())
  , m_center(center)
  , m_type(Type::Point)
  , m_names(std::move(name))
{
  Init();
}

TestFeature::TestFeature(m2::RectD const & boundary, StringUtf8Multilang name)
  : TestFeature(MakePoly(boundary), std::move(name), Type::Area)
{
  Init();
}

TestFeature::TestFeature(vector<m2::PointD> geometry, StringUtf8Multilang name, Type type)
  : m_id(GenUniqueId())
  , m_center(0, 0)
  , m_geometry(std::move(geometry))
  , m_type(type)
  , m_names(std::move(name))
{
  ASSERT(!m_geometry.empty(), ());
  Init();
}

void TestFeature::Init()
{
  m_metadata.Set(Metadata::FMD_TEST_ID, strings::to_string(m_id));
}

bool TestFeature::Matches(FeatureType & feature) const
{
  auto const sv = feature.GetMetadata(feature::Metadata::FMD_TEST_ID);
  if (sv.empty())
    return false;

  uint64_t id;
  CHECK(strings::to_uint(sv, id), (sv));
  return id == m_id;
}

void TestFeature::Serialize(FeatureBuilder & fb) const
{
  using feature::Metadata;

  // Iterate [1, FMD_COUNT). Absent types don't matter here.
  size_t i = 1;
  auto constexpr count = static_cast<size_t>(Metadata::EType::FMD_COUNT);
  for (; i < count; ++i)
  {
    auto const type = static_cast<Metadata::EType>(i);
    if (m_metadata.Has(type))
      fb.GetMetadata().Set(type, string(m_metadata.Get(type)));
  }

  if (!m_geometry.empty())
    fb.AssignPoints(m_geometry);

  switch (m_type)
  {
  case Type::Point:
  {
    fb.SetCenter(m_center);
    break;
  }
  case Type::Line:
  {
    fb.SetLinear();
    break;
  }
  case Type::Area:
  {
    fb.SetArea();
    break;
  }
  case Type::Unknown: break;
  }

  m_names.ForEach([&](int8_t langCode, string_view name)
  {
    if (!name.empty())
      fb.SetName(langCode, name);
  });

  if (!m_postcode.empty())
    fb.GetParams().SetPostcode(m_postcode);
}

// TestPlace -------------------------------------------------------------------------------------
TestPlace::TestPlace(m2::PointD const & center, string const & name, string const & lang, uint32_t type,
                     uint8_t rank /* = 0 */)
  : TestFeature(center, MakeName(name, lang))
  , m_type(type)
  , m_rank(rank)
{}

TestPlace::TestPlace(m2::PointD const & center, StringUtf8Multilang const & name, uint32_t type, uint8_t rank)
  : TestFeature(center, name)
  , m_type(type)
  , m_rank(rank)
{}

TestPlace::TestPlace(std::vector<m2::PointD> const & boundary, std::string const & name, std::string const & lang,
                     uint32_t type, uint8_t rank)
  : TestFeature(boundary, MakeName(name, lang), Type::Area)
  , m_type(type)
  , m_rank(rank)
{}

void TestPlace::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);
  fb.AddType(m_type);
  fb.GetParams().rank = m_rank;
}

string TestPlace::ToDebugString() const
{
  ostringstream os;
  os << "TestPlace { " << DebugPrint(m_names) << ", " << DebugPrint(m_center) << ", "
     << classif().GetReadableObjectName(m_type) << ", " << int(m_rank) << " }";
  return os.str();
}

void TestPlace::SetType(base::StringIL const & e)
{
  m_type = classif().GetTypeByPath(e);
}

TestCountry::TestCountry(m2::PointD const & center, std::string const & name, std::string const & lang)
  : TestPlace(center, name, lang, classif().GetTypeByPath({"place", "country"}), 170 /* rank */)
{
  // Default rank for Country with population ~ 1.0E7
}

TestState::TestState(m2::PointD const & center, string const & name, string const & lang)
  : TestPlace(center, name, lang, classif().GetTypeByPath({"place", "state"}))
{}

TestSea::TestSea(m2::PointD const & center, std::string const & name, std::string const & lang)
  : TestPlace(center, name, lang, classif().GetTypeByPath({"place", "sea"}))
{}

uint32_t TestCity::GetCityType()
{
  return classif().GetTypeByPath({"place", "city"});
}

TestCity::TestCity(m2::PointD const & center, string const & name, string const & lang, uint8_t rank)
  : TestPlace(center, name, lang, GetCityType(), rank)
{}

TestCity::TestCity(m2::PointD const & center, StringUtf8Multilang const & name, uint8_t rank)
  : TestPlace(center, name, GetCityType(), rank)
{}

TestCity::TestCity(vector<m2::PointD> const & boundary, string const & name, string const & lang, uint8_t rank)
  : TestPlace(boundary, name, lang, GetCityType(), rank)
{}

TestVillage::TestVillage(m2::PointD const & center, string const & name, string const & lang, uint8_t rank)
  : TestPlace(center, name, lang, classif().GetTypeByPath({"place", "village"}), rank)
{}

TestSuburb::TestSuburb(m2::PointD const & center, string const & name, string const & lang)
  : TestPlace(center, name, lang, classif().GetTypeByPath({"place", "suburb"}))
{}

// TestStreet --------------------------------------------------------------------------------------
TestStreet::TestStreet(vector<m2::PointD> const & points, string const & name, string const & lang)
  : TestFeature(points, MakeName(name, lang), Type::Line)
{
  SetType({"highway", "living_street"});
}

TestStreet::TestStreet(vector<m2::PointD> const & points, StringUtf8Multilang const & name)
  : TestFeature(points, name, Type::Line)
{
  SetType({"highway", "living_street"});
}

void TestStreet::SetType(base::StringIL const & e)
{
  m_highwayType = classif().GetTypeByPath(e);
}

void TestStreet::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);

  fb.SetType(m_highwayType);

  fb.GetParams().ref = m_roadNumber;
}

string TestStreet::ToDebugString() const
{
  ostringstream os;
  os << "TestStreet [" << DebugPrint(m_names) << ", " << ::DebugPrint(m_geometry) << "]";
  return os.str();
}

// TestSquare --------------------------------------------------------------------------------------
TestSquare::TestSquare(m2::RectD const & rect, string const & name, string const & lang)
  : TestFeature(rect, MakeName(name, lang))
{}

void TestSquare::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);

  auto const & classificator = classif();
  fb.SetType(classificator.GetTypeByPath({"place", "square"}));
}

string TestSquare::ToDebugString() const
{
  ostringstream os;
  os << "TestSquare [" << DebugPrint(m_names) << "]";
  return os.str();
}

// TestPOI -----------------------------------------------------------------------------------------
TestPOI::TestPOI(m2::PointD const & center, string const & name, string const & lang)
  : TestFeature(center, MakeName(name, lang))
{
  SetTypes({{"railway", "station"}});
}

// static
std::pair<TestPOI, FeatureID> TestPOI::AddWithEditor(osm::Editor & editor, MwmSet::MwmId const & mwmId,
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

  for (uint32_t t : m_types)
    fb.AddType(t);

  auto & params = fb.GetParams();
  if (!m_houseNumber.empty())
    params.AddHouseNumber(m_houseNumber);

  if (!m_streetName.empty())
    params.SetStreet(m_streetName);
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
  types.Assign(m_types.begin(), m_types.end());
  return types;
}

void TestPOI::SetTypes(std::initializer_list<base::StringIL> const & types)
{
  m_types.clear();
  auto const & c = classif();
  for (auto const & e : types)
    m_types.push_back(c.GetTypeByPath(e));
}

// TestMultilingualPOI -----------------------------------------------------------------------------
TestMultilingualPOI::TestMultilingualPOI(m2::PointD const & center, string const & defaultName,
                                         std::map<string, string> const & multilingualNames)
  : TestPOI(center, defaultName, "default")
  , m_multilingualNames(multilingualNames)
{}

void TestMultilingualPOI::Serialize(FeatureBuilder & fb) const
{
  TestPOI::Serialize(fb);

  for (auto const & kv : m_multilingualNames)
    CHECK(fb.AddName(kv.first, kv.second), ("Can't set feature name:", kv.second, "(", kv.first, ")"));
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
TestBuilding::TestBuilding(m2::PointD const & center, string const & name, string const & houseNumber,
                           string const & lang)
  : TestFeature(center, MakeName(name, lang))
  , m_houseNumber(houseNumber)
{}

TestBuilding::TestBuilding(m2::PointD const & center, string const & name, string const & houseNumber,
                           string_view street, string const & lang)
  : TestFeature(center, MakeName(name, lang))
  , m_houseNumber(houseNumber)
{
  m_addr.Set(AddressData::Type::Street, street);
}

TestBuilding::TestBuilding(m2::RectD const & boundary, string const & name, string const & houseNumber,
                           string_view street, string const & lang)
  : TestFeature(boundary, MakeName(name, lang))
  , m_houseNumber(houseNumber)
{
  m_addr.Set(AddressData::Type::Street, street);
}

void TestBuilding::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);

  auto & params = fb.GetParams();
  if (!m_houseNumber.empty())
    params.AddHouseNumber(m_houseNumber);

  params.SetAddress(AddressData(m_addr));

  if (m_type == 0)
    fb.AddType(classif().GetTypeByPath({"building"}));
  else
    fb.AddType(m_type);
}

string TestBuilding::ToDebugString() const
{
  ostringstream os;
  os << "TestBuilding [" << DebugPrint(m_names) << ", " << m_houseNumber << ", " << DebugPrint(m_center) << "]";
  return os.str();
}

// TestPark ----------------------------------------------------------------------------------------
TestPark::TestPark(vector<m2::PointD> const & boundary, string const & name, string const & lang)
  : TestFeature(boundary, MakeName(name, lang), Type::Area)
{}

void TestPark::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);

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
  : TestFeature(points, MakeName(name, lang), Type::Line)
{}

void TestRoad::Serialize(FeatureBuilder & fb) const
{
  TestFeature::Serialize(fb);

  auto const & classificator = classif();
  fb.AddType(classificator.GetTypeByPath({"highway", "road"}));
}

string TestRoad::ToDebugString() const
{
  ostringstream os;
  os << "TestRoad [" << DebugPrint(m_names) << "]";
  return os.str();
}

// Functions ---------------------------------------------------------------------------------------
string DebugPrint(TestFeature const & feature)
{
  return feature.ToDebugString();
}
}  // namespace tests_support
}  // namespace generator
