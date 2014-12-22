#include "../../testing/testing.hpp"

#include "../ftypes_matcher.hpp"

#include "../../indexer/index.hpp"

#include "../../indexer/classificator.hpp"

#include "../../base/logging.hpp"

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
}

UNIT_TEST(IsTypeConformed)
{
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

UNIT_TEST(IsStreetChecker)
{
  TEST(ftypes::IsStreetChecker::Instance()(GetStreetTypes()), ());
  TEST(ftypes::IsStreetChecker::Instance()(GetStreetAndNotStreetTypes()), ());
  TEST(!ftypes::IsStreetChecker::Instance()(GetLinkTypes()), ());
}

UNIT_TEST(IsLinkChecker)
{
  TEST(ftypes::IsLinkChecker::Instance()(GetLinkTypes()), ());
  TEST(!ftypes::IsLinkChecker::Instance()(GetStreetTypes()), ());
}

UNIT_TEST(IsBridgeChecker)
{
  TEST(ftypes::IsBridgeChecker::Instance()(GetBridgeTypes()), ());
  TEST(ftypes::IsBridgeChecker::Instance()(GetBridgeAndTunnelTypes()), ());
  TEST(!ftypes::IsBridgeChecker::Instance()(GetTunnelTypes()), ());
}

UNIT_TEST(IsTunnelChecker)
{
  TEST(ftypes::IsTunnelChecker::Instance()(GetTunnelTypes()), ());
  TEST(ftypes::IsTunnelChecker::Instance()(GetBridgeAndTunnelTypes()), ());
  TEST(!ftypes::IsTunnelChecker::Instance()(GetBridgeTypes()), ());
}
