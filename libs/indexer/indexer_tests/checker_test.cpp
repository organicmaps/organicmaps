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

  ftypes::IsBridgeOrTunnelChecker checker;

  base::StringIL arrYes[] = {
      {"highway", "trunk", "bridge"},
      {"highway", "motorway_link", "tunnel"},
  };
  for (auto const & e : arrYes)
    TEST(checker(c.GetTypeByPath(e)), ());

  base::StringIL arrNo[] = {
      {"highway", "motorway_junction"},
      {"highway", "service", "driveway"},
  };
  for (auto const & e : arrNo)
    TEST(!checker(c.GetTypeByPath(e)), ());
}

UNIT_TEST(IsWayChecker)
{
  classificator::Load();
  auto const & c = classif();
  auto const & checker = ftypes::IsWayChecker::Instance();

  TEST(checker(GetStreetTypes()), ());
  TEST(checker(GetStreetAndNotStreetTypes()), ());

  TEST_EQUAL(checker.GetSearchRank(c.GetTypeByPath({"highway", "cycleway", "bridge"})), ftypes::IsWayChecker::Cycleway,
             ());
  TEST_EQUAL(checker.GetSearchRank(c.GetTypeByPath({"highway", "unclassified", "tunnel"})),
             ftypes::IsWayChecker::Minors, ());

  checker.ForEachType([&checker](uint32_t t) { TEST(checker.GetSearchRank(t) != ftypes::IsWayChecker::Default, ()); });
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

UNIT_TEST(IsRecyclingContainerChecker)
{
  classificator::Load();
  auto const & c = classif();

  auto const & checker = ftypes::IsRecyclingContainerChecker::Instance();
  TEST(checker(c.GetTypeByPath({"amenity", "recycling", "container"})), ());
  TEST(!checker(c.GetTypeByPath({"amenity", "recycling", "centre"})), ());
}

UNIT_TEST(BaseCheckerEx_Smoke)
{
  classificator::Load();
  auto const & c = classif();

  ftypes::BaseCheckerEx checker({{"shop"}, {"amenity", "parking"}, {"boundary", "protected_area", "1"}});
  TEST(checker(c.GetTypeByPath({"shop"})), ());
  TEST(checker(c.GetTypeByPath({"shop", "clothes"})), ());
  TEST(checker(c.GetTypeByPath({"amenity", "parking"})), ());
  TEST(checker(c.GetTypeByPath({"amenity", "parking", "fee"})), ());
  TEST(checker(c.GetTypeByPath({"boundary", "protected_area", "1"})), ());

  TEST(!checker(c.GetTypeByPath({"amenity"})), ());
  TEST(!checker(c.GetTypeByPath({"amenity", "cafe"})), ());
  TEST(!checker(c.GetTypeByPath({"boundary", "protected_area"})), ());
  TEST(!checker(c.GetTypeByPath({"boundary", "protected_area", "2"})), ());
}

}  // namespace checker_test
