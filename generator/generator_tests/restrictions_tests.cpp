#include "testing/testing.hpp"

#include "generator/osm_id.hpp"
#include "generator/restrictions.hpp"

#include "std/vector.hpp"

UNIT_TEST(RestrictionTest_ValidCase)
{
  RestrictionCollector restrictionCollector;

  // Adding restrictions and feature ids to restrictionCollector in mixed order.
  restrictionCollector.AddRestriction({osm::Id::Way(1), osm::Id::Way(2)},
                                      RestrictionCollector::Type::No);
  restrictionCollector.AddFeatureId({osm::Id::Way(3)}, 30 /* featureId */);
  restrictionCollector.AddRestriction({osm::Id::Way(2), osm::Id::Way(3)},
                                      RestrictionCollector::Type::No);
  restrictionCollector.AddFeatureId({osm::Id::Way(1)}, 10 /* featureId */);
  restrictionCollector.AddFeatureId({osm::Id::Way(5)}, 50 /* featureId */);
  restrictionCollector.AddRestriction({osm::Id::Way(5), osm::Id::Way(7)},
                                      RestrictionCollector::Type::Only);
  restrictionCollector.AddFeatureId({osm::Id::Way(7)}, 70 /* featureId */);
  restrictionCollector.AddFeatureId({osm::Id::Way(2)}, 20 /* featureId */);

  // Composing restriction in feature id terms.
  restrictionCollector.ComposeRestrictions();
  restrictionCollector.RemoveInvalidRestrictions();

  // Checking the result.
  TEST(restrictionCollector.CheckCorrectness(), ());

  vector<RestrictionCollector::Restriction> const expectedRestrictions =
  {{RestrictionCollector::Type::No, {10, 20}},
   {RestrictionCollector::Type::No, {20, 30}},
   {RestrictionCollector::Type::Only, {50, 70}}};
  TEST_EQUAL(restrictionCollector.m_restrictions, expectedRestrictions, ());
}

UNIT_TEST(RestrictionTest_InvalidCase)
{
  RestrictionCollector restrictionCollector;
  restrictionCollector.AddFeatureId({osm::Id::Way(0)}, 0 /* featureId */);
  restrictionCollector.AddRestriction({osm::Id::Way(0), osm::Id::Way(1)},
                                      RestrictionCollector::Type::No);
  restrictionCollector.AddFeatureId({osm::Id::Way(2)}, 20 /* featureId */);

  restrictionCollector.ComposeRestrictions();

  TEST(!restrictionCollector.CheckCorrectness(), ());

  vector<RestrictionCollector::Restriction> const expectedRestrictions =
      {{RestrictionCollector::Type::No, {0, RestrictionCollector::kInvalidFeatureId}}};
  TEST_EQUAL(restrictionCollector.m_restrictions, expectedRestrictions, ());

  restrictionCollector.RemoveInvalidRestrictions();
  TEST(restrictionCollector.m_restrictions.empty(), ());
  TEST(restrictionCollector.CheckCorrectness(), ());
}
