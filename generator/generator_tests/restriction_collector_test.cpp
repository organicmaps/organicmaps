#include "testing/testing.hpp"

#include "generator/osm_id.hpp"
#include "generator/restriction_collector.hpp"

#include "routing/routing_serializer.hpp"

#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "base/stl_helpers.hpp"

#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

using namespace platform;
using namespace platform::tests_support;

namespace routing
{
string const kRestrictionTestDir = "test-restrictions";

UNIT_TEST(RestrictionTest_ValidCase)
{
  RestrictionCollector restrictionCollector("" /* restrictionPath */, "" /* featureIdToOsmIdsPath */);
  // Adding feature ids.
  restrictionCollector.AddFeatureId(30 /* featureId */, {3} /* osmIds */);
  restrictionCollector.AddFeatureId(10 /* featureId */, {1} /* osmIds */);
  restrictionCollector.AddFeatureId(50 /* featureId */, {5} /* osmIds */);
  restrictionCollector.AddFeatureId(70 /* featureId */, {7} /* osmIds */);
  restrictionCollector.AddFeatureId(20 /* featureId */, {2} /* osmIds */);

  // Adding restrictions.
  TEST(restrictionCollector.AddRestriction(Restriction::Type::No, {1, 2} /* osmIds */), ());
  TEST(restrictionCollector.AddRestriction(Restriction::Type::No, {2, 3} /* osmIds */), ());
  TEST(restrictionCollector.AddRestriction(Restriction::Type::Only, {5, 7} /* osmIds */), ());
  my::SortUnique(restrictionCollector.m_restrictions);

  // Checking the result.
  TEST(restrictionCollector.IsValid(), ());

  RestrictionVec const expectedRestrictions = {{Restriction::Type::No, {10, 20}},
                                               {Restriction::Type::No, {20, 30}},
                                               {Restriction::Type::Only, {50, 70}}};
  TEST_EQUAL(restrictionCollector.m_restrictions, expectedRestrictions, ());
}

UNIT_TEST(RestrictionTest_InvalidCase)
{
  RestrictionCollector restrictionCollector("" /* restrictionPath */, "" /* featureIdToOsmIdsPath */);
  restrictionCollector.AddFeatureId(0 /* featureId */, {0} /* osmIds */);
  restrictionCollector.AddFeatureId(20 /* featureId */, {2} /* osmIds */);

  TEST(!restrictionCollector.AddRestriction(Restriction::Type::No, {0, 1} /* osmIds */), ());

  TEST(!restrictionCollector.HasRestrictions(), ());
  TEST(restrictionCollector.IsValid(), ());
}

UNIT_TEST(RestrictionTest_ParseRestrictions)
{
  string const kRestrictionName = "restrictions_in_osm_ids.csv";
  string const kRestrictionPath = my::JoinFoldersToPath(kRestrictionTestDir, kRestrictionName);
  string const kRestrictionContent = R"(No, 1, 1,
                                        Only, 0, 2,
                                        Only, 2, 3,
                                        No, 38028428, 38028428
                                        No, 4, 5,)";

  ScopedDir const scopedDir(kRestrictionTestDir);
  ScopedFile const scopedFile(kRestrictionPath, kRestrictionContent);

  RestrictionCollector restrictionCollector("" /* restrictionPath */, "" /* featureIdToOsmIdsPath */);

  Platform const & platform = Platform();

  TEST(restrictionCollector.ParseRestrictions(
           my::JoinFoldersToPath(platform.WritableDir(), kRestrictionPath)),
       ());
  TEST(!restrictionCollector.HasRestrictions(), ());
}

UNIT_TEST(RestrictionTest_ParseFeatureId2OsmIdsMapping)
{
  string const kFeatureIdToOsmIdsName = "feature_id_to_osm_ids.csv";
  string const kFeatureIdToOsmIdsPath =
      my::JoinFoldersToPath(kRestrictionTestDir, kFeatureIdToOsmIdsName);
  string const kFeatureIdToOsmIdsContent = R"(1, 10,
                                              2, 20
                                              779703, 5423239545,
                                              3, 30)";

  ScopedDir const scopedDir(kRestrictionTestDir);
  ScopedFile const scopedFile(kFeatureIdToOsmIdsPath, kFeatureIdToOsmIdsContent);

  RestrictionCollector restrictionCollector("" /* restrictionPath */, "" /* featureIdToOsmIdsPath */);

  Platform const & platform = Platform();
  restrictionCollector.ParseFeatureId2OsmIdsMapping(
      my::JoinFoldersToPath(platform.WritableDir(), kFeatureIdToOsmIdsPath));

  vector<pair<uint64_t, uint32_t>> const expectedOsmIds2FeatureId = {
      {10, 1}, {20, 2}, {30, 3}, {5423239545, 779703}};
  vector<pair<uint64_t, uint32_t>> osmIds2FeatureId(
      restrictionCollector.m_osmIdToFeatureId.cbegin(),
      restrictionCollector.m_osmIdToFeatureId.cend());
  sort(osmIds2FeatureId.begin(), osmIds2FeatureId.end(),
       my::LessBy(&pair<uint64_t, uint32_t>::first));
  TEST_EQUAL(osmIds2FeatureId, expectedOsmIds2FeatureId, ());
}

UNIT_TEST(RestrictionTest_RestrictionCollectorWholeClassTest)
{
  string const kRestrictionName = "restrictions_in_osm_ids.csv";
  string const kRestrictionPath = my::JoinFoldersToPath(kRestrictionTestDir, kRestrictionName);
  string const kRestrictionContent = R"(No, 10, 10,
                                        Only, 10, 20,
                                        Only, 30, 40,)";

  string const kFeatureIdToOsmIdsName = "feature_id_to_osm_ids.csv";
  string const kFeatureIdToOsmIdsPath =
      my::JoinFoldersToPath(kRestrictionTestDir, kFeatureIdToOsmIdsName);
  string const kFeatureIdToOsmIdsContent = R"(1, 10,
                                              2, 20,
                                              3, 30,
                                              4, 40)";

  ScopedDir scopedDir(kRestrictionTestDir);
  ScopedFile restrictionScopedFile(kRestrictionPath, kRestrictionContent);
  ScopedFile mappingScopedFile(kFeatureIdToOsmIdsPath, kFeatureIdToOsmIdsContent);

  Platform const & platform = Platform();
  RestrictionCollector restrictionCollector(
      my::JoinFoldersToPath(platform.WritableDir(), kRestrictionPath),
      my::JoinFoldersToPath(platform.WritableDir(), kFeatureIdToOsmIdsPath));
  TEST(restrictionCollector.IsValid(), ());

  RestrictionVec const & restrictions = restrictionCollector.GetRestrictions();
  TEST(is_sorted(restrictions.cbegin(), restrictions.cend()), ());

  RestrictionVec const expectedRestrictions = {{Restriction::Type::No, {1, 1}},
                                               {Restriction::Type::Only, {1, 2}},
                                               {Restriction::Type::Only, {3, 4}}};
  TEST_EQUAL(restrictions, expectedRestrictions, ());
}
}  // namespace routing
