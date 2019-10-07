#include "testing/testing.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/ftypes_matcher.hpp"

#include "base/logging.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

using namespace std;

namespace
{
size_t const roadArrColumnCount = 3;

vector<uint32_t> GetTypes(char const * arr[][roadArrColumnCount], size_t const recCount)
{
  Classificator const & c = classif();
  vector<uint32_t> types;

  for (size_t i = 0; i < recCount; ++i)
    types.push_back(c.GetTypeByPath(vector<string>(arr[i], arr[i] + roadArrColumnCount)));
  return types;
}

vector<uint32_t> GetTypes(std::vector<std::string> const & t)
{
  Classificator const & c = classif();
  vector<uint32_t> types;

  for (auto const & k : t)
    types.push_back(c.GetTypeByPath({k}));
  return types;
}


vector<uint32_t> GetStreetTypes()
{
  char const * arr[][roadArrColumnCount] =
  {
    { "highway", "trunk", "bridge" },
    { "highway", "tertiary", "tunnel" }
  };
  return GetTypes(arr, ARRAY_SIZE(arr));
}

vector<uint32_t> GetStreetAndNotStreetTypes()
{
  char const * arr[][roadArrColumnCount] =
  {
    { "highway", "trunk", "bridge" },
    { "highway", "primary_link", "tunnel" }
  };
  return GetTypes(arr, ARRAY_SIZE(arr));
}

vector<uint32_t> GetLinkTypes()
{
  char const * arr[][roadArrColumnCount] =
  {
    { "highway", "secondary_link", "bridge" },
    { "highway", "motorway_link", "tunnel" }
  };
  return GetTypes(arr, ARRAY_SIZE(arr));
}

vector<uint32_t> GetBridgeTypes()
{
  char const * arr[][roadArrColumnCount] =
  {
    { "highway", "trunk", "bridge" },
    { "highway", "tertiary", "bridge" }
  };
  return GetTypes(arr, ARRAY_SIZE(arr));
}

vector<uint32_t> GetTunnelTypes()
{
  char const * arr[][roadArrColumnCount] =
  {
    { "highway", "trunk", "tunnel" },
    { "highway", "motorway_link", "tunnel" }
  };
  return GetTypes(arr, ARRAY_SIZE(arr));
}

vector<uint32_t> GetBridgeAndTunnelTypes()
{
  char const * arr[][roadArrColumnCount] =
  {
    { "highway", "trunk", "bridge" },
    { "highway", "motorway_link", "tunnel" }
  };
  return GetTypes(arr, ARRAY_SIZE(arr));
}

uint32_t GetMotorwayJunctionType()
{
  Classificator const & c = classif();
  return c.GetTypeByPath({"highway", "motorway_junction"});
}

vector<uint32_t> GetPoiTypes()
{
  std::vector<std::string> const types = {
    "amenity",
    "shop",
    "tourism",
    "leisure",
    "sport",
    "craft",
    "man_made",
    "emergency",
    "office",
    "historic",
    "railway",
    "highway",
    "aeroway"
  };
  return GetTypes(types);
}

vector<uint32_t> GetAttractionsTypes()
{
  auto const & checker = ftypes::AttractionsChecker::Instance();
  vector<uint32_t> types;
  types.reserve(checker.m_primaryTypes.size() + checker.m_additionalTypes.size());
  for (auto t : checker.m_primaryTypes)
    types.push_back(t);
  for (auto t : checker.m_additionalTypes)
    types.push_back(t);

  return types;
}
}  // namespace

UNIT_TEST(IsTypeConformed)
{
  classificator::Load();

  char const * arr[][roadArrColumnCount] =
  {
    {"highway", "trunk", "bridge"},
    {"highway", "motorway_link", "tunnel"}
  };
  vector<uint32_t> types = GetTypes(arr, ARRAY_SIZE(arr));
  TEST(ftypes::IsTypeConformed(types[0], {"highway", "trunk", "bridge"}), ());
  TEST(ftypes::IsTypeConformed(types[0], {"highway", "trunk"}), ());
  TEST(ftypes::IsTypeConformed(types[1], {"highway", "*", "tunnel"}), ());
  TEST(!ftypes::IsTypeConformed(types[0], {"highway", "trunk", "tunnel"}), ());
  TEST(!ftypes::IsTypeConformed(types[0], {"highway", "abcd", "tunnel"}), ());
  TEST(!ftypes::IsTypeConformed(types[1], {"highway", "efgh", "*"}), ());
  TEST(!ftypes::IsTypeConformed(types[1], {"*", "building", "*"}), ());
}

UNIT_TEST(IsWayChecker)
{
  classificator::Load();

  TEST(ftypes::IsWayChecker::Instance()(GetStreetTypes()), ());
  TEST(ftypes::IsWayChecker::Instance()(GetStreetAndNotStreetTypes()), ());
  // TODO (@y, @m, @vng): need to investigate - do we really need this
  // TEST for absence of links, because IsWayChecker() is used for
  // search only.
  //
  // TEST(!ftypes::IsWayChecker::Instance()(GetLinkTypes()), ());
}

UNIT_TEST(IsLinkChecker)
{
  classificator::Load();

  TEST(ftypes::IsLinkChecker::Instance()(GetLinkTypes()), ());
  TEST(!ftypes::IsLinkChecker::Instance()(GetStreetTypes()), ());
}

UNIT_TEST(IsBridgeChecker)
{
  classificator::Load();

  TEST(ftypes::IsBridgeChecker::Instance()(GetBridgeTypes()), ());
  TEST(ftypes::IsBridgeChecker::Instance()(GetBridgeAndTunnelTypes()), ());
  TEST(!ftypes::IsBridgeChecker::Instance()(GetTunnelTypes()), ());
}

UNIT_TEST(IsTunnelChecker)
{
  classificator::Load();

  TEST(ftypes::IsTunnelChecker::Instance()(GetTunnelTypes()), ());
  TEST(ftypes::IsTunnelChecker::Instance()(GetBridgeAndTunnelTypes()), ());
  TEST(!ftypes::IsTunnelChecker::Instance()(GetBridgeTypes()), ());
}

UNIT_TEST(GetHighwayClassTest)
{
  classificator::Load();

  Classificator const & c = classif();

  feature::TypesHolder types1;
  types1.Add(c.GetTypeByPath({"route", "shuttle_train"}));
  TEST_EQUAL(ftypes::GetHighwayClass(types1), ftypes::HighwayClass::Transported, ());

  feature::TypesHolder types2;
  types2.Add(c.GetTypeByPath({"highway", "motorway_link", "tunnel"}));
  TEST_EQUAL(ftypes::GetHighwayClass(types2), ftypes::HighwayClass::Trunk, ());

  feature::TypesHolder types3;
  types3.Add(c.GetTypeByPath({"highway", "unclassified"}));
  TEST_EQUAL(ftypes::GetHighwayClass(types3), ftypes::HighwayClass::LivingStreet, ());

  feature::TypesHolder types4;
  types4.Add(c.GetTypeByPath({"highway"}));
  TEST_EQUAL(ftypes::GetHighwayClass(types4), ftypes::HighwayClass::Error, ());
}

UNIT_TEST(IsPoiChecker)
{
  classificator::Load();
  Classificator const & c = classif();
  auto const & checker = ftypes::IsPoiChecker::Instance();

  for (auto const & t : GetPoiTypes())
    TEST(checker(t), ());

  TEST(!checker(c.GetTypeByPath({"building"})), ());
}

UNIT_TEST(IsAttractionsChecker)
{
  classificator::Load();
  Classificator const & c = classif();
  auto const & checker = ftypes::AttractionsChecker::Instance();

  for (auto const & t : GetAttractionsTypes())
    TEST(checker(t), ());

  TEST(!checker(c.GetTypeByPath({"route", "shuttle_train"})), ());
}

UNIT_TEST(IsMotorwayJunctionChecker)
{
  classificator::Load();

  TEST(ftypes::IsMotorwayJunctionChecker::Instance()(GetMotorwayJunctionType()), ());
  TEST(!ftypes::IsMotorwayJunctionChecker::Instance()(GetPoiTypes()), ());
}
