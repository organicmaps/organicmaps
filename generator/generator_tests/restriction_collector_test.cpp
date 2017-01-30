#include "testing/testing.hpp"

#include "generator/generator_tests_support/restriction_helpers.hpp"

#include "generator/osm_id.hpp"
#include "generator/restriction_collector.hpp"

#include "routing/restrictions_serialization.hpp"

#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "base/stl_helpers.hpp"

#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

using namespace generator;
using namespace platform;
using namespace platform::tests_support;

namespace routing
{
string const kRestrictionTestDir = "test-restrictions";

UNIT_TEST(RestrictionTest_ValidCase)
{
  RestrictionCollector restrictionCollector("" /* restrictionPath */,
                                            "" /* osmIdsToFeatureIdsPath */);
  // Adding feature ids.
  restrictionCollector.AddFeatureId(30 /* featureId */, 3 /* osmId */);
  restrictionCollector.AddFeatureId(10 /* featureId */, 1 /* osmId */);
  restrictionCollector.AddFeatureId(50 /* featureId */, 5 /* osmId */);
  restrictionCollector.AddFeatureId(70 /* featureId */, 7 /* osmId */);
  restrictionCollector.AddFeatureId(20 /* featureId */, 2 /* osmId */);

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
  RestrictionCollector restrictionCollector("" /* restrictionPath */,
                                            "" /* osmIdsToFeatureIdsPath */);
  restrictionCollector.AddFeatureId(0 /* featureId */, 0 /* osmId */);
  restrictionCollector.AddFeatureId(20 /* featureId */, 2 /* osmId */);

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

  RestrictionCollector restrictionCollector("" /* restrictionPath */,
                                            "" /* osmIdsToFeatureIdsPath */);

  Platform const & platform = Platform();

  TEST(restrictionCollector.ParseRestrictions(
           my::JoinFoldersToPath(platform.WritableDir(), kRestrictionPath)),
       ());
  TEST(!restrictionCollector.HasRestrictions(), ());
}

UNIT_TEST(RestrictionTest_RestrictionCollectorWholeClassTest)
{
  ScopedDir scopedDir(kRestrictionTestDir);

  string const kRestrictionName = "restrictions_in_osm_ids.csv";
  string const kRestrictionPath = my::JoinFoldersToPath(kRestrictionTestDir, kRestrictionName);
  string const kRestrictionContent = R"(No, 10, 10,
                                        Only, 10, 20,
                                        Only, 30, 40,)";
  ScopedFile restrictionScopedFile(kRestrictionPath, kRestrictionContent);

  string const kOsmIdsToFeatureIdsName = "osm_ids_to_feature_ids" OSM2FEATURE_FILE_EXTENSION;
  string const osmIdsToFeatureIdsPath =
      my::JoinFoldersToPath(kRestrictionTestDir, kOsmIdsToFeatureIdsName);
  string const kOsmIdsToFeatureIdsContent = R"(10, 1,
                                               20, 2,
                                               30, 3,
                                               40, 4)";
  Platform const & platform = Platform();
  string const osmIdsToFeatureIdsFullPath =
      my::JoinFoldersToPath(platform.WritableDir(), osmIdsToFeatureIdsPath);
  ReEncodeOsmIdsToFeatureIdsMapping(kOsmIdsToFeatureIdsContent, osmIdsToFeatureIdsFullPath);
  ScopedFile mappingScopedFile(osmIdsToFeatureIdsPath);

  RestrictionCollector restrictionCollector(
      my::JoinFoldersToPath(platform.WritableDir(), kRestrictionPath), osmIdsToFeatureIdsFullPath);
  TEST(restrictionCollector.IsValid(), ());

  RestrictionVec const & restrictions = restrictionCollector.GetRestrictions();
  TEST(is_sorted(restrictions.cbegin(), restrictions.cend()), ());

  RestrictionVec const expectedRestrictions = {{Restriction::Type::No, {1, 1}},
                                               {Restriction::Type::Only, {1, 2}},
                                               {Restriction::Type::Only, {3, 4}}};
  TEST_EQUAL(restrictions, expectedRestrictions, ());
}
}  // namespace routing
