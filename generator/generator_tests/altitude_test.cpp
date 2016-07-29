#include "testing/testing.hpp"

#include "generator/altitude_generator.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "routing/road_graph.hpp"
#include "routing/routing_helpers.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/feature_processor.hpp"
#include "indexer/index.hpp"

#include "geometry/point2d.hpp"

#include "coding/file_name_utils.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "std/string.hpp"

using namespace feature;
using namespace generator;
using namespace platform;
using namespace routing;

namespace
{
// These tests generate mwms with altitude sections and then check if altitudes
// in the mwms are correct. The mwms are initialized with different sets of features.
// There are several restrictions for these features:
// * all of them are linear
// * all coords of these features should be integer (it's necessary for easy implementation of
// MockAltitudeGetter)
// * if accoding to one feature a point has some altitude, the point of another feature
//   with the same coords has to have the same altitude
// @TODO(bykoianko) Add ability to add to the tests not road features without altitude information.

// Directory name for creating test mwm and temporary files.
string const kTestDir = "altitude_generation_test";
// Temporary mwm name for testing.
string const kTestMwm = "test";

struct Point3D
{
  Point3D(int32_t x, int32_t y, TAltitude a) : m_point(x, y), m_altitude(a) {}

  m2::PointI m_point;
  TAltitude m_altitude;
};

using TPoint3DList = vector<Point3D>;

TPoint3DList const kRoad1 = {{0, -1, -1}, {0, 0, 0}, {0, 1, 1}};
TPoint3DList const kRoad2 = {{0, 1, 1}, {5, 1, 1}, {10, 1, 1}};
TPoint3DList const kRoad3 = {{10, 1, 1}, {15, 6, 100}, {20, 11, 110}};
TPoint3DList const kRoad4 = {{-10, 1, -1}, {-20, 6, -100}, {-20, -11, -110}};

class MockAltitudeGetter : public AltitudeGetter
{
public:
  using TMockAltitudes = map<m2::PointI, TAltitude>;

  MockAltitudeGetter(vector<TPoint3DList> const & roads)
  {
    for (TPoint3DList const & geom3D : roads)
    {
      for (size_t i = 0; i < geom3D.size(); ++i)
      {
        auto it = m_altitudes.find(geom3D[i].m_point);
        if (it != m_altitudes.end())
        {
          CHECK_EQUAL(it->second, geom3D[i].m_altitude,
                      ("Point", it->first, "is set with two different altitudes."));
          continue;
        }
        m_altitudes[geom3D[i].m_point] = geom3D[i].m_altitude;
      }
    }
  }

  // AltitudeGetter overrides:
  TAltitude GetAltitude(m2::PointD const & p) override
  {
    m2::PointI const rounded(static_cast<int32_t>(round(p.x)), static_cast<int32_t>(round(p.y)));
    auto const it = m_altitudes.find(rounded);
    if (it == m_altitudes.end())
      return kInvalidAltitude;

    return it->second;
  }

private:

  TMockAltitudes m_altitudes;
};

class MockNoAltitudeGetter : public AltitudeGetter
{
public:
  TAltitude GetAltitude(m2::PointD const &) override
  {
    return kInvalidAltitude;
  }
};

vector<m2::PointD> ExtractPoints(TPoint3DList const & geom3D)
{
  vector<m2::PointD> result;
  for (Point3D const & p : geom3D)
    result.push_back(m2::PointD(p.m_point));
  return result;
}

void BuildMwmWithoutAltitudes(vector<TPoint3DList> const & roads, LocalCountryFile & country)
{
  generator::tests_support::TestMwmBuilder builder(country, feature::DataHeader::country);

  for (TPoint3DList const & geom3D : roads)
    builder.Add(generator::tests_support::TestStreet(ExtractPoints(geom3D), string(), string()));
}

void TestAltitudes(MwmValue const & mwmValue, string const & mwmPath,
                   AltitudeGetter & altitudeGetter)
{
  AltitudeLoader loader(mwmValue);

  auto processor = [&altitudeGetter, &loader](FeatureType const & f, uint32_t const & id) {
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
    {
      TAltitude const fromGetter = altitudeGetter.GetAltitude(f.GetPoint(i));
      TAltitude const expected = (fromGetter == kInvalidAltitude ? kDefaultAltitudeMeters : fromGetter);
      TEST_EQUAL(expected, altitudes[i], ("A wrong altitude"));
    }
  };
  feature::ForEachFromDat(mwmPath, processor);
}

void TestAltitudesBuilding(vector<TPoint3DList> const & roads, AltitudeGetter & altitudeGetter)
{
  classificator::Load();
  Platform & platform = GetPlatform();
  string const testDirFullPath = my::JoinFoldersToPath(platform.WritableDir(), kTestDir);

  // Building mwm without altitude section.
  LocalCountryFile country(testDirFullPath, CountryFile(kTestMwm), 1);
  platform::tests_support::ScopedDir testScopedDir(kTestDir);
  platform::tests_support::ScopedFile testScopedMwm(country.GetPath(MapOptions::Map));
  BuildMwmWithoutAltitudes(roads, country);

  // Adding altitude section to mwm.
  string const mwmPath = my::JoinFoldersToPath(testDirFullPath, kTestMwm + DATA_FILE_EXTENSION);
  BuildRoadAltitudes(mwmPath, altitudeGetter);

  // Reading from mwm and testing altitude information.
  Index index;
  auto const regResult = index.RegisterMap(country);
  TEST_EQUAL(regResult.second, MwmSet::RegResult::Success, ());

  MwmSet::MwmHandle mwmHandle = index.GetMwmHandleById(regResult.first);
  TEST(mwmHandle.IsAlive(), ());

  TestAltitudes(*mwmHandle.GetValue<MwmValue>(), mwmPath, altitudeGetter);
}

void TestBuildingAllFeaturesHaveAltitude(vector<TPoint3DList> const & roads)
{
  MockAltitudeGetter altitudeGetter(roads);
  TestAltitudesBuilding(roads, altitudeGetter);
}

void TestBuildingNoFeatureHasAltitude(vector<TPoint3DList> const & roads)
{
  MockNoAltitudeGetter altitudeGetter;
  TestAltitudesBuilding(roads, altitudeGetter);
}

UNIT_TEST(AltitudeGenerationTest_ZeroFeatures)
{
  vector<TPoint3DList> const roads = {};
  TestBuildingAllFeaturesHaveAltitude(roads);
}

UNIT_TEST(AltitudeGenerationTest_OneRoad)
{
  vector<TPoint3DList> const roads = {kRoad1};
  TestBuildingAllFeaturesHaveAltitude(roads);
}

UNIT_TEST(AltitudeGenerationTest_TwoConnectedRoads)
{
  vector<TPoint3DList> const roads = {kRoad1, kRoad2};
  TestBuildingAllFeaturesHaveAltitude(roads);
}

UNIT_TEST(AltitudeGenerationTest_TwoDisconnectedRoads)
{
  vector<TPoint3DList> const roads = {kRoad1, kRoad3};
  TestBuildingAllFeaturesHaveAltitude(roads);
}

UNIT_TEST(AltitudeGenerationTest_ThreeRoads)
{
  vector<TPoint3DList> const roads = {kRoad1, kRoad2, kRoad3};
  TestBuildingAllFeaturesHaveAltitude(roads);
}

UNIT_TEST(AltitudeGenerationTest_FourRoads)
{
  vector<TPoint3DList> const roads = {kRoad1, kRoad2, kRoad3, kRoad4};
  TestBuildingAllFeaturesHaveAltitude(roads);
}

UNIT_TEST(AltitudeGenerationTest_ZeroFeaturesWithoutAltitude)
{
  vector<TPoint3DList> const roads = {};
  TestBuildingNoFeatureHasAltitude(roads);
}

UNIT_TEST(AltitudeGenerationTest_OneRoadWithoutAltitude)
{
  vector<TPoint3DList> const roads = {kRoad1};
  TestBuildingNoFeatureHasAltitude(roads);
}

UNIT_TEST(AltitudeGenerationTest_FourRoadsWithoutAltitude)
{
  vector<TPoint3DList> const roads = {kRoad1, kRoad2, kRoad3, kRoad4};
  TestBuildingNoFeatureHasAltitude(roads);
}
}  // namespace
