#include "testing/testing.hpp"

#include "search/utils.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/ftypes_matcher.hpp"

#include <string>
#include <vector>

namespace checker_test
{
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

vector<uint32_t> GetStreetTypes()
{
  char const * arr[][roadArrColumnCount] = {{"highway", "trunk", "bridge"}, {"highway", "tertiary", "tunnel"}};
  return GetTypes(arr, ARRAY_SIZE(arr));
}

vector<uint32_t> GetStreetAndNotStreetTypes()
{
  char const * arr[][roadArrColumnCount] = {{"highway", "trunk", "bridge"}, {"highway", "primary_link", "tunnel"}};
  return GetTypes(arr, ARRAY_SIZE(arr));
}

vector<uint32_t> GetLinkTypes()
{
  char const * arr[][roadArrColumnCount] = {{"highway", "secondary_link", "bridge"},
                                            {"highway", "motorway_link", "tunnel"}};
  return GetTypes(arr, ARRAY_SIZE(arr));
}

uint32_t GetMotorwayJunctionType()
{
  Classificator const & c = classif();
  return c.GetTypeByPath({"highway", "motorway_junction"});
}

}  // namespace

UNIT_TEST(IsBridgeOrTunnelChecker)
{
  classificator::Load();
  auto const & c = classif();

  base::StringIL arrYes[] = {
      {"highway", "trunk", "bridge"},
      {"highway", "motorway_link", "tunnel"},
  };
  for (auto const & e : arrYes)
    TEST(ftypes::IsBridgeOrTunnelChecker::Instance()(c.GetTypeByPath(e)), ());

  base::StringIL arrNo[] = {
      {"highway", "motorway_junction"},
      {"highway", "service", "driveway"},
  };
  for (auto const & e : arrNo)
    TEST(!ftypes::IsBridgeOrTunnelChecker::Instance()(c.GetTypeByPath(e)), ());
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
  TEST_EQUAL(ftypes::GetHighwayClass(types4), ftypes::HighwayClass::Undefined, ());
}

UNIT_TEST(IsAttractionsChecker)
{
  classificator::Load();
  Classificator const & c = classif();
  auto const & checker = ftypes::AttractionsChecker::Instance();

  base::StringIL arrExceptions[] = {
      {"tourism", "information"},
      {"amenity", "ranger_station"},
  };
  std::vector<uint32_t> exceptions;
  for (auto e : arrExceptions)
    exceptions.push_back(c.GetTypeByPath(e));

  for (uint32_t const t : search::GetCategoryTypes("sights", "en", GetDefaultCategories()))
    if (!base::IsExist(exceptions, ftype::Trunc(t, 2)))
      TEST(checker(t), (c.GetFullObjectName(t)));
}

UNIT_TEST(IsMotorwayJunctionChecker)
{
  classificator::Load();

  TEST(ftypes::IsMotorwayJunctionChecker::Instance()(GetMotorwayJunctionType()), ());
  TEST(!ftypes::IsMotorwayJunctionChecker::Instance()(GetStreetTypes()), ());
}

}  // namespace checker_test
