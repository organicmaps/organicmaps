#include "testing/testing.hpp"

#include "generator/altitude_generator.hpp"
#include "generator/generator_tests_support/test_feature.hpp"
#include "generator/generator_tests_support/test_mwm_builder.hpp"

#include "routing/road_graph.hpp"
#include "routing/routing_helpers.hpp"

#include "indexer/altitude_loader.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/data_source.hpp"
#include "indexer/feature_processor.hpp"

#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"

#include "platform/country_file.hpp"
#include "platform/platform.hpp"
#include "platform/platform_tests_support/scoped_dir.hpp"
#include "platform/platform_tests_support/scoped_file.hpp"
#include "platform/platform_tests_support/writable_dir_changer.hpp"

#include "base/file_name_utils.hpp"
#include "base/logging.hpp"
#include "base/scope_guard.hpp"

#include "defines.hpp"

#include <map>
#include <string>
#include <vector>

namespace altitude_test
{
using namespace feature;
using namespace generator;
using namespace platform;
using namespace platform::tests_support;
using namespace routing;

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
std::string const kTestDir = "altitude_generation_test";
// Temporary mwm name for testing.
std::string const kTestMwm = "test";

struct Point3D
{
  Point3D(int32_t x, int32_t y, geometry::Altitude a) : m_point(x, y), m_altitude(a) {}

  m2::PointI m_point;
  geometry::Altitude m_altitude;
};

using TPoint3DList = std::vector<Point3D>;

TPoint3DList const kRoad1 = {{0, -1, -1}, {0, 0, 0}, {0, 1, 1}};
TPoint3DList const kRoad2 = {{0, 1, 1}, {5, 1, 1}, {10, 1, 1}};
TPoint3DList const kRoad3 = {{10, 1, 1}, {15, 6, 100}, {20, 11, 110}};
TPoint3DList const kRoad4 = {{-10, 1, -1}, {-20, 6, -100}, {-20, -11, -110}};

class MockAltitudeGetter : public AltitudeGetter
{
public:
  using TMockAltitudes = std::map<m2::PointI, geometry::Altitude>;

  explicit MockAltitudeGetter(std::vector<TPoint3DList> const & roads)
  {
    for (TPoint3DList const & geom3D : roads)
    {
      for (auto const & g : geom3D)
      {
        auto const it = m_altitudes.find(g.m_point);
        if (it != m_altitudes.cend())
        {
          CHECK_EQUAL(it->second, g.m_altitude, ("Point", it->first, "is set with two different altitudes."));
          continue;
        }
        m_altitudes[g.m_point] = g.m_altitude;
      }
    }
  }

  // AltitudeGetter overrides:
  geometry::Altitude GetAltitude(m2::PointD const & p) override
  {
    m2::PointI const rounded(static_cast<int32_t>(round(p.x)), static_cast<int32_t>(round(p.y)));
    auto const it = m_altitudes.find(rounded);
    if (it == m_altitudes.end())
      return geometry::kInvalidAltitude;

    return it->second;
  }

private:
  TMockAltitudes m_altitudes;
};

class MockNoAltitudeGetter : public AltitudeGetter
{
public:
  geometry::Altitude GetAltitude(m2::PointD const &) override { return geometry::kInvalidAltitude; }
};

std::vector<m2::PointD> ExtractPoints(TPoint3DList const & geom3D)
{
  std::vector<m2::PointD> result;
  for (Point3D const & p : geom3D)
    result.push_back(m2::PointD(p.m_point));
  return result;
}

void BuildMwmWithoutAltitudes(std::vector<TPoint3DList> const & roads, LocalCountryFile & country)
{
  generator::tests_support::TestMwmBuilder builder(country, feature::DataHeader::MapType::Country);

  for (TPoint3DList const & geom3D : roads)
    builder.Add(generator::tests_support::TestStreet(ExtractPoints(geom3D), std::string(), std::string()));
}

void TestAltitudes(DataSource const & dataSource, MwmSet::MwmId const & mwmId, std::string const & mwmPath,
                   bool hasAltitudeExpected, AltitudeGetter & expectedAltitudes)
{
  auto const handle = dataSource.GetMwmHandleById(mwmId);
  TEST(handle.IsAlive(), ());
  AltitudeLoaderCached loader(*handle.GetValue());
  TEST_EQUAL(loader.HasAltitudes(), hasAltitudeExpected, ());

  auto processor = [&expectedAltitudes, &loader](FeatureType & f, uint32_t const & id)
  {
    f.ParseGeometry(FeatureType::BEST_GEOMETRY);
    size_t const pointsCount = f.GetPointsCount();
    geometry::Altitudes const & altitudes = loader.GetAltitudes(id, pointsCount);

    if (!routing::IsRoad(feature::TypesHolder(f)))
    {
      TEST(altitudes.empty(), ());
      return;
    }

    TEST_EQUAL(altitudes.size(), pointsCount, ());

    for (size_t i = 0; i < pointsCount; ++i)
    {
      geometry::Altitude const fromGetter = expectedAltitudes.GetAltitude(f.GetPoint(i));
      geometry::Altitude const expected =
          (fromGetter == geometry::kInvalidAltitude ? geometry::kDefaultAltitudeMeters : fromGetter);
      TEST_EQUAL(expected, altitudes[i], ("A wrong altitude"));
    }
  };
  feature::ForEachFeature(mwmPath, processor);
}

void TestAltitudesBuilding(std::vector<TPoint3DList> const & roads, bool hasAltitudeExpected,
                           AltitudeGetter & altitudeGetter)
{
  classificator::Load();
  Platform & platform = GetPlatform();
  std::string const testDirFullPath = base::JoinPath(platform.WritableDir(), kTestDir);

  // Building mwm without altitude section.
  LocalCountryFile country(testDirFullPath, CountryFile(kTestMwm), 1);
  ScopedDir testScopedDir(kTestDir);
  ScopedFile testScopedMwm(base::JoinPath(kTestDir, kTestMwm + DATA_FILE_EXTENSION), ScopedFile::Mode::Create);

  BuildMwmWithoutAltitudes(roads, country);

  // Adding altitude section to mwm.
  auto const mwmPath = testScopedMwm.GetFullPath();
  BuildRoadAltitudes(mwmPath, altitudeGetter);

  // Reading from mwm and testing altitude information.
  FrozenDataSource dataSource;
  auto const regResult = dataSource.RegisterMap(country);
  TEST_EQUAL(regResult.second, MwmSet::RegResult::Success, ());

  TestAltitudes(dataSource, regResult.first /* mwmId */, mwmPath, hasAltitudeExpected, altitudeGetter);
}

void TestBuildingAllFeaturesHaveAltitude(std::vector<TPoint3DList> const & roads, bool hasAltitudeExpected)
{
  MockAltitudeGetter altitudeGetter(roads);
  TestAltitudesBuilding(roads, hasAltitudeExpected, altitudeGetter);
}

void TestBuildingNoFeatureHasAltitude(std::vector<TPoint3DList> const & roads, bool hasAltitudeExpected)
{
  MockNoAltitudeGetter altitudeGetter;
  TestAltitudesBuilding(roads, hasAltitudeExpected, altitudeGetter);
}

UNIT_TEST(AltitudeGenerationTest_ZeroFeatures)
{
  std::vector<TPoint3DList> const roads = {};
  TestBuildingAllFeaturesHaveAltitude(roads, false /* hasAltitudeExpected */);
}

UNIT_TEST(AltitudeGenerationTest_OneRoad)
{
  std::vector<TPoint3DList> const roads = {kRoad1};
  TestBuildingAllFeaturesHaveAltitude(roads, true /* hasAltitudeExpected */);
}

UNIT_TEST(AltitudeGenerationTest_TwoConnectedRoads)
{
  std::vector<TPoint3DList> const roads = {kRoad1, kRoad2};
  TestBuildingAllFeaturesHaveAltitude(roads, true /* hasAltitudeExpected */);
}

UNIT_TEST(AltitudeGenerationTest_TwoDisconnectedRoads)
{
  std::vector<TPoint3DList> const roads = {kRoad1, kRoad3};
  TestBuildingAllFeaturesHaveAltitude(roads, true /* hasAltitudeExpected */);
}

UNIT_TEST(AltitudeGenerationTest_ThreeRoads)
{
  std::vector<TPoint3DList> const roads = {kRoad1, kRoad2, kRoad3};
  TestBuildingAllFeaturesHaveAltitude(roads, true /* hasAltitudeExpected */);
}

UNIT_TEST(AltitudeGenerationTest_FourRoads)
{
  std::vector<TPoint3DList> const roads = {kRoad1, kRoad2, kRoad3, kRoad4};
  TestBuildingAllFeaturesHaveAltitude(roads, true /* hasAltitudeExpected */);
}

UNIT_TEST(AltitudeGenerationTest_ZeroFeaturesWithoutAltitude)
{
  std::vector<TPoint3DList> const roads = {};
  TestBuildingNoFeatureHasAltitude(roads, false /* hasAltitudeExpected */);
}

UNIT_TEST(AltitudeGenerationTest_OneRoadWithoutAltitude)
{
  std::vector<TPoint3DList> const roads = {kRoad1};
  TestBuildingNoFeatureHasAltitude(roads, false /* hasAltitudeExpected */);
}

UNIT_TEST(AltitudeGenerationTest_FourRoadsWithoutAltitude)
{
  std::vector<TPoint3DList> const roads = {kRoad1, kRoad2, kRoad3, kRoad4};
  TestBuildingNoFeatureHasAltitude(roads, false /* hasAltitudeExpected */);
}
}  // namespace altitude_test
