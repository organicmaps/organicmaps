#include "testing/testing.hpp"

#include "drape_frontend/area_pattern.hpp"
#include "drape_frontend/stylist.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"

UNIT_TEST(Stylist_IsHatching)
{
  classificator::Load();
  auto const & cl = classif();

  auto const & checker = df::IsHatchingTerritoryChecker::Instance();

  TEST(checker(cl.GetTypeByPath({"boundary", "protected_area", "1"})), ());
  TEST(!checker(cl.GetTypeByPath({"boundary", "protected_area", "2"})), ());
  TEST(!checker(cl.GetTypeByPath({"boundary", "protected_area"})), ());

  TEST(checker(cl.GetTypeByPath({"boundary", "national_park"})), ());

  TEST(checker(cl.GetTypeByPath({"landuse", "military", "danger_area"})), ());

  TEST(checker(cl.GetTypeByPath({"amenity", "prison"})), ());
}

UNIT_TEST(Stylist_IsAreaPattern)
{
  classificator::Load();
  auto const & cl = classif();

  // Constructing the singleton resolves every checker type via GetTypeByPath, which CHECKs the type
  // exists - so an invalid path (e.g. the 3-level natural=beach=sand mistaken for 2-level) fails here.
  auto const & checker = df::IsAreaPatternChecker::Instance();

  // Stipple: sandy / desert surfaces and intermittent water. natural=sand is a beach subtype, matched via the parent.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "beach"})), df::AreaPattern::Stipple, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "beach", "sand"})), df::AreaPattern::Stipple, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "desert"})), df::AreaPattern::Stipple, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "water", "intermittent"})), df::AreaPattern::Stipple, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "basin", "intermittent"})), df::AreaPattern::Stipple, ());

  // Speckle: rocky surfaces.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "scree"})), df::AreaPattern::Speckle, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "bare_rock"})), df::AreaPattern::Speckle, ());

  // Grid: planted landuse.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "orchard"})), df::AreaPattern::Grid, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "vineyard"})), df::AreaPattern::Grid, ());

  // Permanent water and unrelated area types get no pattern.
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"natural", "water"})), df::AreaPattern::None, ());
  TEST_EQUAL(checker.GetPattern(cl.GetTypeByPath({"landuse", "basin"})), df::AreaPattern::None, ());
}
