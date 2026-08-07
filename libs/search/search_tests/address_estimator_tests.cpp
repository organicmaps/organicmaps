#include "search/address_estimator.hpp"

#include "geometry/mercator.hpp"

#include "testing/testing.hpp"

#include <string>

namespace address_estimator_tests
{
using namespace search;

Result MakeAddress(double lat, double lon, std::string const & name, std::string const & address = "Surrey, Canada")
{
  Result result(mercator::FromLatLon(lat, lon), name);
  result.SetAddress(std::string(address));
  result.SetType(Result::Type::LatLon);
  return result;
}

Result const * FindEstimated(Results const & results)
{
  for (auto const & result : results)
  {
    if (result.IsEstimatedAddress())
      return &result;
  }
  return nullptr;
}

UNIT_TEST(AddressEstimator_ExtrapolatesOneConsistentInterval)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.1207362, -122.8575047, "6480, 131A Street"));
  source.AddResultNoChecks(MakeAddress(49.1208601, -122.8575937, "6486, 131A Street"));
  source.AddResultNoChecks(MakeAddress(49.1209318, -122.8577086, "6492, 131A Street"));

  auto const results = MakeEstimatedAddressResults("6498 131a st Surrey", source);
  auto const * estimated = FindEstimated(results);
  TEST(estimated, ());
  TEST_EQUAL(estimated->GetString(), "6498, 131A Street", ());
  auto const latLon = mercator::ToLatLon(estimated->GetFeatureCenter());
  TEST_ALMOST_EQUAL_ABS(latLon.m_lat, 49.1210035, 1e-7, ());
  TEST_ALMOST_EQUAL_ABS(latLon.m_lon, -122.8578235, 1e-7, ());
  TEST_EQUAL(results.GetCount(), 1, ());
}

UNIT_TEST(AddressEstimator_IgnoresSimilarStreetNames)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.1209472, -122.8591754, "6498, 131 Street"));
  source.AddResultNoChecks(MakeAddress(49.1208601, -122.8575937, "6486, 131A Street"));
  source.AddResultNoChecks(MakeAddress(49.1209318, -122.8577086, "6492, 131A Street"));

  auto const results = MakeEstimatedAddressResults("6498 131A Street", source);
  auto const * estimated = FindEstimated(results);
  TEST(estimated, ());
  TEST_EQUAL(estimated->GetString(), "6498, 131A Street", ());
  TEST_EQUAL(results.GetCount(), 2, ());
  TEST_EQUAL(results[1].GetString(), "6498, 131 Street", ());
}

UNIT_TEST(AddressEstimator_PreservesExactAddress)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.1208601, -122.8575937, "6486, 131A Street"));
  source.AddResultNoChecks(MakeAddress(49.1209318, -122.8577086, "6492, 131A Street"));
  source.AddResultNoChecks(MakeAddress(49.1210035, -122.8578235, "6498, 131A Street"));

  auto const results = MakeEstimatedAddressResults("6498 131A Street", source);
  TEST_EQUAL(results.GetCount(), 1, ());
  TEST_EQUAL(results[0].GetString(), "6498, 131A Street", ());
  TEST(!results[0].IsEstimatedAddress(), ());
}

UNIT_TEST(AddressEstimator_RequiresTwoNearbySupports)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.1209318, -122.8577086, "6492, 131A Street"));

  auto const results = MakeEstimatedAddressResults("6498 131A Street", source);
  TEST_EQUAL(results.GetCount(), 0, ());
}

UNIT_TEST(AddressEstimator_RejectsExcessiveExtrapolation)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.1207362, -122.8575047, "6480, 131A Street"));
  source.AddResultNoChecks(MakeAddress(49.1208601, -122.8575937, "6486, 131A Street"));

  auto const results = MakeEstimatedAddressResults("6498 131A Street", source);
  TEST_EQUAL(results.GetCount(), 0, ());
}
}  // namespace address_estimator_tests
