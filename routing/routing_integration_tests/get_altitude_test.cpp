#include "testing/testing.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_altitude.hpp"
#include "indexer/feature_data.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/index.hpp"

#include "routing/routing_helpers.hpp"

#include "geometry/mercator.hpp"

#include "platform/local_country_file.hpp"

#include "base/math.hpp"

#include "std/string.hpp"
#include "std/unique_ptr.hpp"
#include "std/utility.hpp"

namespace
{
using namespace feature;

void TestAltitudeOfAllMwmFeatures(string const & countryId, TAltitude const altitudeLowerBoundMeters,
                                  TAltitude const altitudeUpperBoundMeters)
{
  Index index;
  platform::LocalCountryFile const country = platform::LocalCountryFile::MakeForTesting(countryId);
  TEST_NOT_EQUAL(country.GetFiles(), MapOptions::Nothing, (country));
  pair<MwmSet::MwmId, MwmSet::RegResult> const regResult = index.RegisterMap(country);

  TEST_EQUAL(regResult.second, MwmSet::RegResult::Success, ());
  TEST(regResult.first.IsAlive(), ());

  MwmSet::MwmHandle handle = index.GetMwmHandleById(regResult.first);
  TEST(handle.IsAlive(), ());

  MwmValue * mwmValue = handle.GetValue<MwmValue>();
  TEST(mwmValue != nullptr, ());
  unique_ptr<AltitudeLoader> altitudeLoader = make_unique<AltitudeLoader>(*mwmValue);

  classificator::Load();
  classif().SortClassificator();

  ForEachFromDat(country.GetPath(MapOptions::Map), [&](FeatureType const & f, uint32_t const & id)
  {
    if (!routing::IsRoad(TypesHolder(f)))
      return;

    f.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const pointsCount = f.GetPointsCount();
    if (pointsCount == 0)
      return;

    TAltitudes altitudes = altitudeLoader->GetAltitudes(id, pointsCount);
    TEST(!altitudes.empty(),
         ("Empty altitude vector. MWM:", countryId, ", feature id:", id, ", altitudes:", altitudes));

    for (auto const altitude : altitudes)
    {
      TEST_EQUAL(my::clamp(altitude, altitudeLowerBoundMeters, altitudeUpperBoundMeters), altitude,
                 ("Unexpected altitude. MWM:", countryId, ", feature id:", id, ", altitudes:", altitudes));
    }
  });
}

UNIT_TEST(AllMwmFeaturesGetAltitudeTest)
{
  TestAltitudeOfAllMwmFeatures("Russia_Moscow", 50 /* altitudeLowerBoundMeters */,
                               300 /* altitudeUpperBoundMeters */);
  TestAltitudeOfAllMwmFeatures("Nepal_Kathmandu", 250 /* altitudeLowerBoundMeters */,
                               6000 /* altitudeUpperBoundMeters */);
  TestAltitudeOfAllMwmFeatures("Netherlands_North Holland_Amsterdam", -25 /* altitudeLowerBoundMeters */,
                               50 /* altitudeUpperBoundMeters */);
}
}  // namespace
