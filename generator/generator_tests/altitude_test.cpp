#include "testing/testing.hpp"

#include "generator/altitude_generator.hpp"

#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "routing/road_graph.hpp"
#include "routing/routing_helpers.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/index.hpp"
#include "indexer/feature_processor.hpp"

#include "geometry/point2d.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"

#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "std/string.hpp"

using namespace feature;
using namespace generator;
using namespace platform;
using namespace routing;
using namespace tests_support;

namespace
{
// These tests generate mwms with altitude sections and then check if altitudes
// in the mwms are correct. The mwms are inited with different sets of features defined below.
// There are several restrictions for these features:
// * all of them are linear
// * all coords of these features should be integer (it's necessary for easy implementation of MockAltitudeGetter)
// * if accoding to one feature a point has some altitude, the point of another feature
//   with the same coords has to have the same altitude
// @TODO(bykoianko) Add ability to add to the tests not road features without altitude information.

// Directory name for creating test mwm and temporary files.
string const kTestDir = "altitude_generation_test";
// Temporary mwm name for testing.
string const kTestMwm = "test";

struct Rounded3DPoint
{
  Rounded3DPoint(int32_t x, int32_t y, TAltitude a) : point2D(x, y), altitude(a) {}

  m2::PointI point2D;
  TAltitude altitude;
};

using TRounded3DGeom = vector<Rounded3DPoint>;

TRounded3DGeom const kRoad1 = {{0, -1, -1}, {0, 0, 0}, {0, 1, 1}};
TRounded3DGeom const kRoad2 = {{0, 1, 1}, {5, 1, 1}, {10, 1, 1}};
TRounded3DGeom const kRoad3 = {{10, 1, 1}, {15, 6, 100}, {20, 11, 110}};
TRounded3DGeom const kRoad4 = {{-10, 1, -1}, {-20, 6, -100}, {-20, -11, -110}};

class MockAltitudeGetter : public IAltitudeGetter
{
public:
  using TMockAltitudes = map<m2::PointI, TAltitude>;

  MockAltitudeGetter(TMockAltitudes && altitudes) : m_altitudes(altitudes) {}

  // IAltitudeGetter overrides:
  TAltitude GetAltitude(m2::PointD const & p) override
  {
    m2::PointI const rounded(static_cast<int32_t>(round(p.x)), static_cast<int32_t>(round(p.y)));
    auto const it = m_altitudes.find(rounded);
    if (it == m_altitudes.end())
      return kInvalidAltitude;

    return it->second;
  }

private:
  TMockAltitudes const & m_altitudes;
};

template<class T>
vector<T> ConvertTo2DGeom(TRounded3DGeom const & geom3D)
{
  vector<T> dv;
  dv.clear();
  for (Rounded3DPoint const & p : geom3D)
    dv.push_back(T(p.point2D));
  return dv;
}

TAltitudes ConvertToAltitudes(TRounded3DGeom const & geom3D)
{
  TAltitudes altitudes;
  altitudes.clear();
  for (Rounded3DPoint const & p : geom3D)
    altitudes.push_back(p.altitude);
  return altitudes;
}

void FillAltitudes(vector<TRounded3DGeom> const & roadFeatures,
                   MockAltitudeGetter::TMockAltitudes & altitudes)
{
  for (TRounded3DGeom const & geom3D : roadFeatures)
  {
    vector<m2::PointI> const featureGeom = ConvertTo2DGeom<m2::PointI>(geom3D);
    TAltitudes const featureAlt = ConvertToAltitudes(geom3D);
    CHECK_EQUAL(featureGeom.size(), featureAlt.size(), ());
    for (size_t i = 0; i < featureGeom.size(); ++i)
    {
      auto it = altitudes.find(featureGeom[i]);
      if (it != altitudes.end())
      {
        CHECK_EQUAL(it->second, featureAlt[i], ("Point", it->first, "is set with two different altitudes."));
        continue;
      }
      altitudes[featureGeom[i]] = featureAlt[i];
    }
  }
}

void BuildMwmWithoutAltitude(vector<TRounded3DGeom> const & roadFeatures, LocalCountryFile & country)
{
  TestMwmBuilder builder(country, feature::DataHeader::country);

  for (TRounded3DGeom const & geom3D : roadFeatures)
    builder.Add(TestStreet(ConvertTo2DGeom<m2::PointD>(geom3D), string(), string()));
}

void ReadAndTestAltitudeInfo(MwmValue const & mwmValue, string const & mwmPath, MockAltitudeGetter & altitudeGetter)
{
  AltitudeLoader const loader(mwmValue);

  auto processor = [&altitudeGetter, &loader](FeatureType const & f, uint32_t const & id)
  {
    f.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const pointsCount = f.GetPointsCount();
    TAltitudes const altitudes = loader.GetAltitudes(id, pointsCount);

    if (!routing::IsRoad(feature::TypesHolder(f)))
    {
      TEST(altitudes.empty(), ());
      return;
    }

    TEST_EQUAL(altitudes.size(), pointsCount, ());

    for (size_t i = 0; i < pointsCount; ++i)
      TEST_EQUAL(altitudeGetter.GetAltitude(f.GetPoint(i)), altitudes[i], ("A wrong altitude"));
  };
  feature::ForEachFromDat(mwmPath, processor);
}

void TestAltitudeSection(vector<TRounded3DGeom> const & roadFeatures)
{
  classificator::Load();
  Platform & platform = GetPlatform();
  string const testDirFullPath = my::JoinFoldersToPath(platform.WritableDir(), kTestDir);

  MY_SCOPE_GUARD(removeTmpDir, [&] ()
  {
    GetPlatform().RmDirRecursively(testDirFullPath);
  });

  // Building mwm without altitude section.
  platform.MkDir(testDirFullPath);
  LocalCountryFile country(testDirFullPath, CountryFile(kTestMwm), 1);
  BuildMwmWithoutAltitude(roadFeatures, country);

  // Creating MockAltitudeGetter.
  MockAltitudeGetter::TMockAltitudes altitudes;
  FillAltitudes(roadFeatures, altitudes);
  MockAltitudeGetter altitudeGetter(move(altitudes));

  // Adding altitude section to mwm.
  string const mwmPath = my::JoinFoldersToPath(testDirFullPath, kTestMwm + DATA_FILE_EXTENSION);
  BuildRoadAltitudes(mwmPath, altitudeGetter);

  // Reading from mwm and testing altitue information.
  Index index;
  pair<MwmSet::MwmId, MwmSet::RegResult> regResult = index.RegisterMap(country);
  TEST_EQUAL(regResult.second, MwmSet::RegResult::Success, ());

  MwmSet::MwmHandle mwmHandle = index.GetMwmHandleById(regResult.first);
  CHECK(mwmHandle.IsAlive(), ());

  ReadAndTestAltitudeInfo(*mwmHandle.GetValue<MwmValue>(), mwmPath, altitudeGetter);
}
} // namespace

UNIT_TEST(ZeroFeatures_AltitudeGenerationTest)
{
  vector<TRounded3DGeom> const roadFeatures = {};
  TestAltitudeSection(roadFeatures);
}

UNIT_TEST(OneRoad_AltitudeGenerationTest)
{
  vector<TRounded3DGeom> const roadFeatures = {kRoad1};
  TestAltitudeSection(roadFeatures);
}

UNIT_TEST(TwoConnectedRoads_AltitudeGenerationTest)
{
  vector<TRounded3DGeom> const roadFeatures = {kRoad1, kRoad2};
  TestAltitudeSection(roadFeatures);
}

UNIT_TEST(TwoDisconnectedRoads_AltitudeGenerationTest)
{
  vector<TRounded3DGeom> const roadFeatures = {kRoad1, kRoad3};
  TestAltitudeSection(roadFeatures);
}

UNIT_TEST(ThreeRoads_AltitudeGenerationTest)
{
  vector<TRounded3DGeom> const roadFeatures = {kRoad1, kRoad2, kRoad3};
  TestAltitudeSection(roadFeatures);
}

UNIT_TEST(FourRoads_AltitudeGenerationTest)
{
  vector<TRounded3DGeom> const roadFeatures = {kRoad1, kRoad2, kRoad3, kRoad4};
  TestAltitudeSection(roadFeatures);
}
