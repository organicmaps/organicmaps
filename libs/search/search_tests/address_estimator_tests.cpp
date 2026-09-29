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
    if (result.IsEstimatedAddress())
      return &result;
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
  TEST_EQUAL(results.GetCount(), 1, ());
  TEST_EQUAL(results[0].GetString(), "6492, 131A Street", ());
}

UNIT_TEST(AddressEstimator_RejectsExcessiveExtrapolation)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.1207362, -122.8575047, "6480, 131A Street"));
  source.AddResultNoChecks(MakeAddress(49.1208601, -122.8575937, "6486, 131A Street"));

  auto const results = MakeEstimatedAddressResults("6498 131A Street", source);
  TEST_EQUAL(results.GetCount(), 2, ());
  TEST(!FindEstimated(results), ());
}

UNIT_TEST(AddressEstimator_RequiresTheCompleteStreetName)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.1207362, -122.8575047, "6480, Main Street"));
  source.AddResultNoChecks(MakeAddress(49.1208601, -122.8575937, "6486, Main Street"));

  auto const results = MakeEstimatedAddressResults("6490 Main Street West", source);
  TEST_EQUAL(results.GetCount(), 2, ());
  TEST(!FindEstimated(results), ());
}

UNIT_TEST(AddressEstimator_RejectsUnrelatedResultForContactMarker)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.20538, -122.770653, "16291, 111A Avenue"));

  auto const results = MakeEstimatedAddressResults("11378 158A Street Surrey", source);
  TEST_EQUAL(results.GetCount(), 1, ());
  TEST(!IsAddressResultMatchingQuery("11378 158A Street Surrey", results[0]), ());
}

UNIT_TEST(AddressEstimator_AcceptsExactAndEstimatedContactMarkers)
{
  Results source;
  source.AddResultNoChecks(MakeAddress(49.1207362, -122.8575047, "6480, 131A Street"));
  source.AddResultNoChecks(MakeAddress(49.1208601, -122.8575937, "6486, 131A Street"));
  source.AddResultNoChecks(MakeAddress(49.1209318, -122.8577086, "6492, 131A Street"));

  auto const estimated = MakeEstimatedAddressResults("6498 131A Street", source);
  TEST_EQUAL(estimated.GetCount(), 1, ());
  TEST(IsAddressResultMatchingQuery("6498 131A Street", estimated[0]), ());

  auto const exact = MakeAddress(49.1209318, -122.8577086, "6492, 131A Street");
  TEST(IsAddressResultMatchingQuery("6492 131A Street", exact), ());
}
UNIT_TEST(AddressEstimator_ContactRequiresCompleteStreetIndependentlyOfLocality)
{
  auto const wrong = MakeAddress(0, 0, "123, Main Street West", "Springfield, Illinois, USA");
  TEST(!IsAddressResultMatchingQuery("123 Main Street Springfield Illinois USA", wrong, "123 Main Street"), ());
  auto const correct = MakeAddress(0, 0, "123, Main Street", "Springfield, Illinois, United States");
  TEST(IsAddressResultMatchingQuery("123 Main Street Springfield Illinois USA", correct, "123 Main Street"), ());
  TEST(!IsAddressResultMatchingQuery("123 Main Street Springfield Massachusetts USA", correct, "123 Main Street"), ());
  Results supports;
  supports.AddResultNoChecks(MakeAddress(0, 0, "120, Main Street", "West Springfield"));
  supports.AddResultNoChecks(MakeAddress(0, 0.0001, "122, Main Street", "West Springfield"));
  TEST(
      !FindEstimated(MakeEstimatedAddressResults("124 Main Street West Springfield", supports, "124 Main Street West")),
      ());
}

UNIT_TEST(AddressEstimator_ContactFormattedFrenchAndNumericOverflow)
{
  auto const result = MakeAddress(48.855, 2.36, "12, Rue de Rivoli", "Paris, France");
  TEST(IsAddressResultMatchingQuery("12 Rue de Rivoli Paris France", result, "12 Rue de Rivoli"), ());
  TEST(!IsAddressResultMatchingQuery("999999999999999999999999 Rue de Rivoli Paris France", result,
                                     "999999999999999999999999 Rue de Rivoli"),
       ());
}

UNIT_TEST(AddressEstimator_ContactStreetFirstMapAddresses)
{
  auto const exact = MakeAddress(49.91, -97.17, "Ingersoll Street, 910", "Winnipeg, Manitoba, Canada");
  TEST(IsAddressResultMatchingQuery("910 Ingersoll Street Winnipeg Manitoba Canada", exact, "910 Ingersoll Street"),
       ());
  TEST(!IsAddressResultMatchingQuery("910 Ingersoll Street West Winnipeg Manitoba Canada", exact,
                                     "910 Ingersoll Street West"),
       ());
  Results supports;
  supports.AddResultNoChecks(MakeAddress(49.1208601, -122.8575937, "131A Street, 6486"));
  supports.AddResultNoChecks(MakeAddress(49.1209318, -122.8577086, "131A Street, 6492"));
  supports.SetEndMarker(true);
  auto const estimated = MakeEstimatedAddressResults("6498 131A Street Surrey Canada", supports, "6498 131A Street");
  auto const * result = FindEstimated(estimated);
  TEST(result, ());
  TEST(IsAddressResultMatchingQuery("6498 131A Street Surrey Canada", *result, "6498 131A Street"), ());
}
}  // namespace address_estimator_tests
