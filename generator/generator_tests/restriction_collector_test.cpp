#include "testing/testing.hpp"

#include "generator/generator_tests_support/routing_helpers.hpp"

#include "generator/osm_id.hpp"
#include "generator/restriction_collector.hpp"

#include "routing/restrictions_serialization.hpp"

#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"

#include "platform/platform.hpp"

#include "coding/file_name_utils.hpp"

#include "base/stl_helpers.hpp"

#include <string>
#include <utility>
#include <vector>

using namespace generator;
using namespace platform;
using namespace platform::tests_support;

namespace routing
{
std::string const kRestrictionTestDir = "test-restrictions";

UNIT_TEST(RestrictionTest_ValidCase)
{
  RestrictionCollector restrictionCollector;
  // Adding feature ids.
  restrictionCollector.AddFeatureId(30 /* featureId */, osm::Id::Way(3));
  restrictionCollector.AddFeatureId(10 /* featureId */, osm::Id::Way(1));
  restrictionCollector.AddFeatureId(50 /* featureId */, osm::Id::Way(5));
  restrictionCollector.AddFeatureId(70 /* featureId */, osm::Id::Way(7));
  restrictionCollector.AddFeatureId(20 /* featureId */, osm::Id::Way(2));

  // Adding restrictions.
  TEST(restrictionCollector.AddRestriction(Restriction::Type::No, {osm::Id::Way(1), osm::Id::Way(2)}), ());
  TEST(restrictionCollector.AddRestriction(Restriction::Type::No, {osm::Id::Way(2), osm::Id::Way(3)}), ());
  TEST(restrictionCollector.AddRestriction(Restriction::Type::Only, {osm::Id::Way(5), osm::Id::Way(7)}), ());
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
  RestrictionCollector restrictionCollector;
  restrictionCollector.AddFeatureId(0 /* featureId */, osm::Id::Way(0));
  restrictionCollector.AddFeatureId(20 /* featureId */, osm::Id::Way(2));

  TEST(!restrictionCollector.AddRestriction(Restriction::Type::No, {osm::Id::Way(0), osm::Id::Way(1)}), ());

  TEST(!restrictionCollector.HasRestrictions(), ());
  TEST(restrictionCollector.IsValid(), ());
}

UNIT_TEST(RestrictionTest_ParseRestrictions)
{
  std::string const kRestrictionName = "restrictions_in_osm_ids.csv";
  std::string const kRestrictionPath = my::JoinPath(kRestrictionTestDir, kRestrictionName);
  std::string const kRestrictionContent = R"(No, 1, 1,
                                        Only, 0, 2,
                                        Only, 2, 3,
                                        No, 38028428, 38028428
                                        No, 4, 5,)";

  ScopedDir const scopedDir(kRestrictionTestDir);
  ScopedFile const scopedFile(kRestrictionPath, kRestrictionContent);

  RestrictionCollector restrictionCollector;

  Platform const & platform = Platform();

  TEST(restrictionCollector.ParseRestrictions(
           my::JoinPath(platform.WritableDir(), kRestrictionPath)),
       ());
  TEST(!restrictionCollector.HasRestrictions(), ());
}

UNIT_TEST(RestrictionTest_RestrictionCollectorWholeClassTest)
{
  ScopedDir scopedDir(kRestrictionTestDir);

  std::string const kRestrictionName = "restrictions_in_osm_ids.csv";
  std::string const kRestrictionPath = my::JoinPath(kRestrictionTestDir, kRestrictionName);
  std::string const kRestrictionContent = R"(No, 10, 10,
                                        Only, 10, 20,
                                        Only, 30, 40,)";
  ScopedFile restrictionScopedFile(kRestrictionPath, kRestrictionContent);

  std::string const kOsmIdsToFeatureIdsName = "osm_ids_to_feature_ids" OSM2FEATURE_FILE_EXTENSION;
  std::string const osmIdsToFeatureIdsPath =
      my::JoinPath(kRestrictionTestDir, kOsmIdsToFeatureIdsName);
  std::string const kOsmIdsToFeatureIdsContent = R"(10, 1,
                                               20, 2,
                                               30, 3,
                                               40, 4)";
  Platform const & platform = Platform();
  ScopedFile mappingScopedFile(osmIdsToFeatureIdsPath);
  std::string const osmIdsToFeatureIdsFullPath = mappingScopedFile.GetFullPath();
  ReEncodeOsmIdsToFeatureIdsMapping(kOsmIdsToFeatureIdsContent, osmIdsToFeatureIdsFullPath);

  RestrictionCollector restrictionCollector(my::JoinPath(platform.WritableDir(), kRestrictionPath),
                                            osmIdsToFeatureIdsFullPath);
  TEST(restrictionCollector.IsValid(), ());

  RestrictionVec const & restrictions = restrictionCollector.GetRestrictions();
  TEST(is_sorted(restrictions.cbegin(), restrictions.cend()), ());

  RestrictionVec const expectedRestrictions = {{Restriction::Type::No, {1, 1}},
                                               {Restriction::Type::Only, {1, 2}},
                                               {Restriction::Type::Only, {3, 4}}};
  TEST_EQUAL(restrictions, expectedRestrictions, ());
}
}  // namespace routing
