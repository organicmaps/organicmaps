#include "testing/testing.hpp"

#include "routing_common/motorcycle_model.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_data.hpp"

#include <array>
#include <initializer_list>
#include <memory>

namespace motorcycle_model_test
{
using namespace routing;

class MotorcycleModelTest
{
public:
  MotorcycleModelTest()
  {
    classificator::Load();
    auto const & c = classif();

    motorway = c.GetTypeByPath({"highway", "motorway"});
    trunk = c.GetTypeByPath({"highway", "trunk"});
    primary = c.GetTypeByPath({"highway", "primary"});
    secondary = c.GetTypeByPath({"highway", "secondary"});
    tertiary = c.GetTypeByPath({"highway", "tertiary"});
    unclassified = c.GetTypeByPath({"highway", "unclassified"});
    track = c.GetTypeByPath({"highway", "track"});
    path = c.GetTypeByPath({"highway", "path"});
    cycleway = c.GetTypeByPath({"highway", "cycleway"});
    footway = c.GetTypeByPath({"highway", "footway"});
    pedestrian = c.GetTypeByPath({"highway", "pedestrian"});
    steps = c.GetTypeByPath({"highway", "steps"});

    noCar = c.GetTypeByPath({"hwtag", "nocar"});
    yesCar = c.GetTypeByPath({"hwtag", "yescar"});
    pavedGood = c.GetTypeByPath({"psurface", "paved_good"});
    pavedBad = c.GetTypeByPath({"psurface", "paved_bad"});
    unpavedGood = c.GetTypeByPath({"psurface", "unpaved_good"});
    unpavedBad = c.GetTypeByPath({"psurface", "unpaved_bad"});
  }

  static feature::TypesHolder MakeRoad(std::initializer_list<uint32_t> types)
  {
    feature::TypesHolder holder(feature::GeomType::Line);
    for (uint32_t t : types)
      holder.Add(t);
    return holder;
  }

  static SpeedParams OutCityParams(MaxspeedType maxSpeed = kInvalidSpeed) { return {maxSpeed, false /* inCity */}; }

  uint32_t motorway, trunk, primary, secondary, tertiary, unclassified;
  uint32_t track, path, cycleway, footway, pedestrian, steps;
  uint32_t noCar, yesCar, pavedGood, pavedBad, unpavedGood, unpavedBad;
};

UNIT_CLASS_TEST(MotorcycleModelTest, AStarAdmissibilityAllProfiles)
{
  std::array<MotorcycleProfile, 5> constexpr kProfiles = {MotorcycleProfile::Default, MotorcycleProfile::Touring,
                                                          MotorcycleProfile::Fun, MotorcycleProfile::Offroad,
                                                          MotorcycleProfile::HardEnduro};
  std::array<uint32_t, 5> const surfaces = {0 /* none */, pavedGood, pavedBad, unpavedGood, unpavedBad};
  std::array<MaxspeedType, 4> constexpr kMaxSpeeds = {kInvalidSpeed, 50, 130, 200};

  auto const & c = classif();
  for (auto const profile : kProfiles)
  {
    auto const & model = MotorcycleModel::AllLimitsInstance(profile);
    double const maxWeightSpeed = model.GetMaxWeightSpeed();
    TEST_EQUAL(maxWeightSpeed, 200.0, (profile));

    auto const & offroadSpeed = model.GetOffroadSpeed();
    TEST_GREATER(offroadSpeed.m_weight, 0.0, (profile));
    TEST_LESS_OR_EQUAL(offroadSpeed.m_weight, maxWeightSpeed, (profile));
    TEST_EQUAL(offroadSpeed.m_eta, kNotUsed, (profile));

    for (HighwayType const limit : MotorcycleModel::GetOptions(profile))
    {
      uint32_t const hwType = c.GetTypeForIndex(static_cast<uint32_t>(limit));
      for (uint32_t const surface : surfaces)
      {
        feature::TypesHolder holder(feature::GeomType::Line);
        holder.Add(hwType);
        if (surface != 0)
          holder.Add(surface);

        for (bool const inCity : {false, true})
        {
          for (MaxspeedType const maxSpeed : kMaxSpeeds)
          {
            SpeedKMpH const speed = model.GetSpeed(holder, {maxSpeed, inCity});
            TEST(speed.IsValid(), (profile, limit, inCity, maxSpeed, speed));
            TEST_LESS_OR_EQUAL(speed.m_weight, maxWeightSpeed, (profile, limit, inCity, maxSpeed, speed));
            TEST_LESS_OR_EQUAL(speed.m_eta, maxWeightSpeed, (profile, limit, inCity, maxSpeed, speed));
          }
        }
      }
    }
  }
}

UNIT_CLASS_TEST(MotorcycleModelTest, QuickProfilePrefersMotorwayAndTrunk)
{
  auto const & quickModel = MotorcycleModel::AllLimitsInstance(MotorcycleProfile::Default);
  auto const params = OutCityParams();

  SpeedKMpH const quickMotorway = quickModel.GetSpeed(MakeRoad({motorway, pavedGood}), params);
  SpeedKMpH const quickTrunk = quickModel.GetSpeed(MakeRoad({trunk, pavedGood}), params);
  SpeedKMpH const quickPrimary = quickModel.GetSpeed(MakeRoad({primary, pavedGood}), params);
  SpeedKMpH const quickSecondary = quickModel.GetSpeed(MakeRoad({secondary, pavedGood}), params);
  SpeedKMpH const quickTertiary = quickModel.GetSpeed(MakeRoad({tertiary, pavedGood}), params);
  SpeedKMpH const quickUnclassified = quickModel.GetSpeed(MakeRoad({unclassified, pavedGood}), params);

  // Quick strongly prefers motorway/trunk over primary and lower road classes (~2x weight ratio over primary).
  TEST_GREATER(quickMotorway.m_weight, quickTrunk.m_weight, (quickMotorway, quickTrunk));
  TEST_GREATER(quickTrunk.m_weight, quickPrimary.m_weight, (quickTrunk, quickPrimary));
  TEST_GREATER(quickMotorway.m_weight, 2.0 * quickPrimary.m_weight, (quickMotorway, quickPrimary));
  TEST_GREATER(quickPrimary.m_weight, quickSecondary.m_weight, (quickPrimary, quickSecondary));
  TEST_GREATER(quickSecondary.m_weight, quickTertiary.m_weight, (quickSecondary, quickTertiary));
  TEST_GREATER(quickTertiary.m_weight, quickUnclassified.m_weight, (quickTertiary, quickUnclassified));
}

UNIT_CLASS_TEST(MotorcycleModelTest, TouringProfilePrefersPavedMainRoadsOverMotorway)
{
  auto const & defaultModel = MotorcycleModel::AllLimitsInstance(MotorcycleProfile::Default);
  auto const & touringModel = MotorcycleModel::AllLimitsInstance(MotorcycleProfile::Touring);
  auto const params = OutCityParams();

  auto const motorwayRoad = MakeRoad({motorway, pavedGood});
  auto const trunkRoad = MakeRoad({trunk, pavedGood});
  auto const primaryGood = MakeRoad({primary, pavedGood});
  auto const primaryBad = MakeRoad({primary, pavedBad});
  auto const secondaryGood = MakeRoad({secondary, pavedGood});
  auto const unclassifiedGood = MakeRoad({unclassified, pavedGood});
  auto const trackGood = MakeRoad({track, unpavedGood});

  SpeedKMpH const touringMotorway = touringModel.GetSpeed(motorwayRoad, params);
  SpeedKMpH const touringTrunk = touringModel.GetSpeed(trunkRoad, params);
  SpeedKMpH const touringPrimary = touringModel.GetSpeed(primaryGood, params);
  SpeedKMpH const touringPrimaryBad = touringModel.GetSpeed(primaryBad, params);
  SpeedKMpH const touringSecondary = touringModel.GetSpeed(secondaryGood, params);
  SpeedKMpH const touringUnclassified = touringModel.GetSpeed(unclassifiedGood, params);
  SpeedKMpH const touringTrack = touringModel.GetSpeed(trackGood, params);

  // Touring favors smooth paved trunk/primary/secondary over motorway, rough asphalt, unclassified, and tracks.
  TEST_GREATER(touringTrunk.m_weight, touringMotorway.m_weight, (touringTrunk, touringMotorway));
  TEST_GREATER(touringPrimary.m_weight, touringMotorway.m_weight, (touringPrimary, touringMotorway));
  TEST_GREATER(touringSecondary.m_weight, touringMotorway.m_weight, (touringSecondary, touringMotorway));
  TEST_GREATER(touringPrimary.m_weight, touringPrimaryBad.m_weight, (touringPrimary, touringPrimaryBad));
  TEST_GREATER(touringSecondary.m_weight, touringUnclassified.m_weight, (touringSecondary, touringUnclassified));
  TEST_GREATER(touringUnclassified.m_weight, touringTrack.m_weight, (touringUnclassified, touringTrack));

  // Even a 130 km/h motorway stays below paved primary/secondary in weight while keeping realistic ETA.
  SpeedKMpH const touringMotorway130 = touringModel.GetSpeed(motorwayRoad, OutCityParams(130));
  TEST_GREATER(touringPrimary.m_weight, touringMotorway130.m_weight, (touringPrimary, touringMotorway130));
  TEST_GREATER(touringSecondary.m_weight, touringMotorway130.m_weight, (touringSecondary, touringMotorway130));
  TEST_ALMOST_EQUAL_ABS(touringMotorway.m_eta, defaultModel.GetSpeed(motorwayRoad, params).m_eta, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(touringPrimary.m_eta, defaultModel.GetSpeed(primaryGood, params).m_eta, 1e-6, ());
}

UNIT_CLASS_TEST(MotorcycleModelTest, FunProfilePrefersBackroadsOverMotorway)
{
  auto const & defaultModel = MotorcycleModel::AllLimitsInstance(MotorcycleProfile::Default);
  auto const & funModel = MotorcycleModel::AllLimitsInstance(MotorcycleProfile::Fun);
  auto const params = OutCityParams();

  auto const motorwayRoad = MakeRoad({motorway, pavedGood});
  auto const trunkRoad = MakeRoad({trunk, pavedGood});
  auto const secondaryRoad = MakeRoad({secondary, pavedGood});
  auto const tertiaryRoad = MakeRoad({tertiary, pavedGood});
  auto const unclassifiedRoad = MakeRoad({unclassified, pavedGood});

  SpeedKMpH const funMotorway = funModel.GetSpeed(motorwayRoad, params);
  SpeedKMpH const funTrunk = funModel.GetSpeed(trunkRoad, params);
  SpeedKMpH const funSecondary = funModel.GetSpeed(secondaryRoad, params);
  SpeedKMpH const funTertiary = funModel.GetSpeed(tertiaryRoad, params);
  SpeedKMpH const funUnclassified = funModel.GetSpeed(unclassifiedRoad, params);

  // Fun profile prioritizes secondary, tertiary, and unclassified roads over motorway and trunk.
  TEST_GREATER(funSecondary.m_weight, funMotorway.m_weight, (funSecondary, funMotorway));
  TEST_GREATER(funTertiary.m_weight, funMotorway.m_weight, (funTertiary, funMotorway));
  TEST_GREATER(funUnclassified.m_weight, funMotorway.m_weight, (funUnclassified, funMotorway));
  TEST_GREATER(funUnclassified.m_weight, funTrunk.m_weight, (funUnclassified, funTrunk));

  // Even on a 130 km/h maxspeed motorway, Fun still prefers tertiary and unclassified backroads.
  SpeedKMpH const funMotorway130 = funModel.GetSpeed(motorwayRoad, OutCityParams(130));
  TEST_GREATER(funTertiary.m_weight, funMotorway130.m_weight, (funTertiary, funMotorway130));
  TEST_GREATER(funUnclassified.m_weight, funMotorway130.m_weight, (funUnclassified, funMotorway130));

  // Penalties only affect m_weight; m_eta matches the Default profile's realistic speeds.
  TEST_LESS(funMotorway.m_weight, funMotorway.m_eta, (funMotorway));
  TEST_ALMOST_EQUAL_ABS(funMotorway.m_eta, defaultModel.GetSpeed(motorwayRoad, params).m_eta, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(funTrunk.m_eta, defaultModel.GetSpeed(trunkRoad, params).m_eta, 1e-6, ());
  TEST_ALMOST_EQUAL_ABS(funTertiary.m_eta, defaultModel.GetSpeed(tertiaryRoad, params).m_eta, 1e-6, ());
}

UNIT_CLASS_TEST(MotorcycleModelTest, OffroadAndHardEnduroPreferencesAndRealEta)
{
  auto const & defaultModel = MotorcycleModel::AllLimitsInstance(MotorcycleProfile::Default);
  auto const & offroadModel = MotorcycleModel::AllLimitsInstance(MotorcycleProfile::Offroad);
  auto const & enduroModel = MotorcycleModel::AllLimitsInstance(MotorcycleProfile::HardEnduro);
  auto const params = OutCityParams();

  auto const motorwayPaved = MakeRoad({motorway, pavedGood});
  auto const trunkPaved = MakeRoad({trunk, pavedGood});
  auto const primaryPaved = MakeRoad({primary, pavedGood});
  auto const secondaryPaved = MakeRoad({secondary, pavedGood});
  auto const primaryUntagged = MakeRoad({primary});
  auto const secondaryUntagged = MakeRoad({secondary});
  auto const unclassifiedPaved = MakeRoad({unclassified, pavedGood});
  auto const unclassifiedUnpaved = MakeRoad({unclassified, unpavedGood});
  auto const trackUnpavedGood = MakeRoad({track, unpavedGood});
  auto const trackUnpavedBad = MakeRoad({track, unpavedBad});

  SpeedKMpH const defaultMotorway = defaultModel.GetSpeed(motorwayPaved, params);

  // Offroad profile: tracks significantly beat paved primary/secondary/motorway in m_weight (~2x+ even
  // against untagged asphalt), while m_eta stays realistic.
  SpeedKMpH const offroadMotorway = offroadModel.GetSpeed(motorwayPaved, params);
  SpeedKMpH const offroadPrimary = offroadModel.GetSpeed(primaryPaved, params);
  SpeedKMpH const offroadSecondary = offroadModel.GetSpeed(secondaryPaved, params);
  SpeedKMpH const offroadPrimaryUntagged = offroadModel.GetSpeed(primaryUntagged, params);
  SpeedKMpH const offroadSecondaryUntagged = offroadModel.GetSpeed(secondaryUntagged, params);
  SpeedKMpH const offroadTrackGood = offroadModel.GetSpeed(trackUnpavedGood, params);
  SpeedKMpH const offroadTrackBad = offroadModel.GetSpeed(trackUnpavedBad, params);
  SpeedKMpH const offroadUnclPaved = offroadModel.GetSpeed(unclassifiedPaved, params);
  SpeedKMpH const offroadUnclUnpaved = offroadModel.GetSpeed(unclassifiedUnpaved, params);

  TEST_GREATER(offroadTrackGood.m_weight, 2.0 * offroadPrimary.m_weight, (offroadTrackGood, offroadPrimary));
  TEST_GREATER(offroadTrackGood.m_weight, 2.0 * offroadSecondary.m_weight, (offroadTrackGood, offroadSecondary));
  TEST_GREATER(offroadTrackGood.m_weight, 2.0 * offroadPrimaryUntagged.m_weight,
               (offroadTrackGood, offroadPrimaryUntagged));
  TEST_GREATER(offroadTrackGood.m_weight, 2.0 * offroadSecondaryUntagged.m_weight,
               (offroadTrackGood, offroadSecondaryUntagged));
  TEST_GREATER(offroadTrackBad.m_weight, offroadMotorway.m_weight, (offroadTrackBad, offroadMotorway));
  TEST_GREATER(offroadUnclUnpaved.m_weight, offroadUnclPaved.m_weight, (offroadUnclUnpaved, offroadUnclPaved));
  TEST_GREATER(offroadUnclPaved.m_eta, offroadUnclUnpaved.m_eta, (offroadUnclPaved, offroadUnclUnpaved));
  TEST_LESS(offroadTrackGood.m_eta, offroadPrimary.m_eta, (offroadTrackGood, offroadPrimary));
  TEST_ALMOST_EQUAL_ABS(offroadMotorway.m_eta, defaultMotorway.m_eta, 1e-6, ());

  // MWM car default speed on tracks must not overwrite Offroad/HardEnduro boosted m_weight or collapse m_eta.
  SpeedParams const mwmTrackDefault(kInvalidSpeed, 25 /* defSpeedKmPH */, false /* inCity */);
  SpeedKMpH const offroadTrackWithMwmDef = offroadModel.GetSpeed(trackUnpavedGood, mwmTrackDefault);
  TEST_ALMOST_EQUAL_ABS(offroadTrackWithMwmDef.m_weight, offroadTrackGood.m_weight, 1e-6, (offroadTrackWithMwmDef));
  TEST_ALMOST_EQUAL_ABS(offroadTrackWithMwmDef.m_eta, offroadTrackGood.m_eta, 1e-6, (offroadTrackWithMwmDef));

  // HardEnduro profile: tracks aggressively beat motorway, trunk, primary, and secondary in m_weight,
  // while motorway ETA remains uncorrupted and track ETA stays realistic.
  SpeedKMpH const enduroMotorway = enduroModel.GetSpeed(motorwayPaved, params);
  SpeedKMpH const enduroTrunk = enduroModel.GetSpeed(trunkPaved, params);
  SpeedKMpH const enduroPrimary = enduroModel.GetSpeed(primaryPaved, params);
  SpeedKMpH const enduroSecondary = enduroModel.GetSpeed(secondaryPaved, params);
  SpeedKMpH const enduroTrackBad = enduroModel.GetSpeed(trackUnpavedBad, params);
  SpeedKMpH const enduroTrackWithMwmDef = enduroModel.GetSpeed(trackUnpavedBad, mwmTrackDefault);

  TEST_GREATER(enduroTrackBad.m_weight, enduroMotorway.m_weight, (enduroTrackBad, enduroMotorway));
  TEST_GREATER(enduroTrackBad.m_weight, enduroTrunk.m_weight, (enduroTrackBad, enduroTrunk));
  TEST_GREATER(enduroTrackBad.m_weight, 5.0 * enduroPrimary.m_weight, (enduroTrackBad, enduroPrimary));
  TEST_GREATER(enduroTrackBad.m_weight, 5.0 * enduroSecondary.m_weight, (enduroTrackBad, enduroSecondary));
  TEST_ALMOST_EQUAL_ABS(enduroTrackWithMwmDef.m_weight, enduroTrackBad.m_weight, 1e-6, (enduroTrackWithMwmDef));
  TEST_ALMOST_EQUAL_ABS(enduroTrackWithMwmDef.m_eta, enduroTrackBad.m_eta, 1e-6, (enduroTrackWithMwmDef));
  TEST_ALMOST_EQUAL_ABS(enduroMotorway.m_eta, defaultMotorway.m_eta, 1e-6, ());
  TEST_LESS(enduroTrackBad.m_eta, enduroTrackBad.m_weight, (enduroTrackBad));

  // Verify maxspeed-tagged paved roads retain the full asphalt penalty in Offroad and HardEnduro
  // and are not weakened by VehicleModel::m_minSurfaceFactorForMaxspeed.
  SpeedKMpH const offroadPrimary80 = offroadModel.GetSpeed(primaryPaved, OutCityParams(80));
  TEST_ALMOST_EQUAL_ABS(offroadPrimary80.m_weight, 80.0 * 0.42 * 0.45, 1e-6, (offroadPrimary80));
  TEST_GREATER(offroadTrackGood.m_weight, 2.0 * offroadPrimary80.m_weight, (offroadTrackGood, offroadPrimary80));

  SpeedKMpH const enduroMotorway130 = enduroModel.GetSpeed(motorwayPaved, OutCityParams(130));
  TEST_ALMOST_EQUAL_ABS(enduroMotorway130.m_weight, 130.0 * 0.08 * 0.25, 1e-6, (enduroMotorway130));
  TEST_ALMOST_EQUAL_ABS(enduroMotorway130.m_eta, 130.0 * 0.93 * 1.0, 1e-6, (enduroMotorway130));
}

UNIT_CLASS_TEST(MotorcycleModelTest, RoadAccessAndFactory)
{
  std::array<MotorcycleProfile, 5> constexpr kProfiles = {MotorcycleProfile::Default, MotorcycleProfile::Touring,
                                                          MotorcycleProfile::Fun, MotorcycleProfile::Offroad,
                                                          MotorcycleProfile::HardEnduro};

  auto const trackRoad = MakeRoad({track});
  auto const pathRoad = MakeRoad({path});
  auto const blockedMotorway = MakeRoad({motorway, noCar});

  for (auto const profile : kProfiles)
  {
    MotorcycleModelFactory factory([](std::string const &) { return std::string(); }, profile);
    auto const defaultModel = std::dynamic_pointer_cast<MotorcycleModel>(factory.GetVehicleModel());
    auto const countryModel = std::dynamic_pointer_cast<MotorcycleModel>(factory.GetVehicleModelForCountry("Serbia"));

    TEST(defaultModel != nullptr, (profile));
    TEST(countryModel != nullptr, (profile));
    TEST_EQUAL(defaultModel->GetProfile(), profile, ());
    TEST_EQUAL(countryModel->GetProfile(), profile, ());

    TEST(defaultModel->IsRoad(trackRoad), (profile));
    TEST(!defaultModel->IsRoad(MakeRoad({pedestrian})), (profile));
    TEST(!defaultModel->IsRoad(MakeRoad({footway})), (profile));
    TEST(!defaultModel->IsRoad(MakeRoad({steps})), (profile));
    TEST(!defaultModel->IsRoad(MakeRoad({cycleway})), (profile));
    TEST(!defaultModel->IsRoad(blockedMotorway), (profile));

    // HighwayPath is not indexed in the VehicleType::Car MWM graph unless tagged hwtag=yescar.
    TEST(!defaultModel->IsRoad(pathRoad), (profile));
    TEST(defaultModel->IsRoad(MakeRoad({path, yesCar})), (profile));
  }
}
}  // namespace motorcycle_model_test
