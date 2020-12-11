#include "routing/routing_integration_tests/routing_test_tools.hpp"

#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/routing_manager.hpp"

#include "routing/routing_callbacks.hpp"
#include "routing/routing_options.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/mercator.hpp"

#include <memory>
#include <set>
#include <string>

using namespace routing;

namespace
{
class TestAbsentRegionsFinder
{
public:
  TestAbsentRegionsFinder();

  void TestRegions(Checkpoints const & checkpoints, std::set<std::string> const & planRegions);

protected:
  std::set<std::string> GetRegions(Checkpoints const & checkpoints);

  FrameworkParams m_frameworkParams;
  Framework m_framework;
  RoutingManager & m_manager;
  RoutingManager::Callbacks & m_callbacks;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  CountryFileGetterFn m_countryFileGetter;
  LocalFileCheckerFn m_localFileChecker;
};

TestAbsentRegionsFinder::TestAbsentRegionsFinder()
  : m_framework(m_frameworkParams)
  , m_manager(m_framework.GetRoutingManager())
  , m_callbacks(m_manager.GetCallbacksForTests())
{
  storage::Storage storage;
  storage.RegisterAllLocalMaps(false /* enableDiffs */);
  m_numMwmIds = CreateNumMwmIds(storage);

  m_countryFileGetter = [this](m2::PointD const & p) -> std::string {
    return m_callbacks.m_countryInfoGetter().GetRegionCountryId(p);
  };

  m_localFileChecker = [&](std::string const & countryFile) {
    MwmSet::MwmId const mwmId =
        m_callbacks.m_dataSourceGetter().GetMwmIdByCountryFile(platform::CountryFile(countryFile));
    if (!mwmId.IsAlive())
      return false;

    return version::MwmTraits(mwmId.GetInfo()->m_version).HasRoutingIndex();
  };
}

void TestAbsentRegionsFinder::TestRegions(Checkpoints const & checkpoints,
                                          std::set<std::string> const & planRegions)
{
  std::set<std::string> const & factRegions = GetRegions(checkpoints);
  TEST_EQUAL(planRegions, factRegions, ());
}

std::set<std::string> TestAbsentRegionsFinder::GetRegions(Checkpoints const & checkpoints)
{
  AbsentRegionsFinder finder(m_countryFileGetter, m_localFileChecker, m_numMwmIds,
                             m_callbacks.m_dataSourceGetter());
  RouterDelegate delegate;

  finder.GenerateAbsentRegions(checkpoints, delegate);

  std::set<std::string> regions;
  finder.GetAllRegions(regions);

  return regions;
}

// From "Russia_Republic of Karelia_South" to "Russia_Krasnodar Krai".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, TestAbsentRegionsFinder_Karelia_Krasnodar)
{
  Checkpoints const checkpoints{mercator::FromLatLon(61.76, 34.45),
                                mercator::FromLatLon(45.07, 38.94)};

  std::set<std::string> const planRegions{"Russia_Krasnodar Krai",
                                          "Russia_Leningradskaya Oblast_Southeast",
                                          "Russia_Lipetsk Oblast",
                                          "Russia_Moscow",
                                          "Russia_Moscow Oblast_East",
                                          "Russia_Moscow Oblast_West",
                                          "Russia_Republic of Karelia_South",
                                          "Russia_Rostov Oblast",
                                          "Russia_Tula Oblast",
                                          "Russia_Vologda Oblast",
                                          "Russia_Voronezh Oblast",
                                          "Ukraine_Luhansk Oblast"};

  TestRegions(checkpoints, planRegions);
}

// From "Canada_Ontario_Kingston" to "US_Maryland_and_DC".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, TestAbsentRegionsFinder_Kingston_DC)
{
  Checkpoints const checkpoints{mercator::FromLatLon(45.38, -75.69),
                                mercator::FromLatLon(38.91, -77.031)};

  std::set<std::string> const planRegions{"Canada_Ontario_Kingston", "US_Maryland_Baltimore",
                                          "US_Maryland_and_DC",      "US_New York_North",
                                          "US_New York_West",        "US_Pennsylvania_Central",
                                          "US_Pennsylvania_Scranton"};

  TestRegions(checkpoints, planRegions);
}

// From "US_Colorado_Aspen" to "Canada_Saskatchewan_Saskatoon".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, TestAbsentRegionsFinder_Colorado_Saskatchewan)
{
  Checkpoints const checkpoints{mercator::FromLatLon(39.95763, -106.79994),
                                mercator::FromLatLon(49.92034, -106.99302)};

  std::set<std::string> const planRegions{"US_Colorado_Aspen", "Canada_Ontario_Northwestern",
                                          "US_Illinois_Springfield", "US_Wisconsin_Eau Claire",
                                          "Canada_Saskatchewan_Saskatoon"};

  TestRegions(checkpoints, planRegions);
}

// From "Belgium_Flemish Brabant" to "Germany_North Rhine-Westphalia_Regierungsbezirk Koln_Aachen".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, TestAbsentRegionsFinder_Belgium_Germany)
{
  Checkpoints const checkpoints{mercator::FromLatLon(50.87763, 4.44676),
                                mercator::FromLatLon(50.76935, 6.42488)};

  std::set<std::string> const planRegions{
      "Belgium_Flemish Brabant", "Belgium_Liege", "Belgium_Limburg",
      "Germany_North Rhine-Westphalia_Regierungsbezirk Koln_Aachen", "Netherlands_Limburg"};

  TestRegions(checkpoints, planRegions);
}

// From "Germany_North Rhine-Westphalia_Regierungsbezirk Koln_Aachen" to "Belgium_Flemish Brabant".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, TestAbsentRegionsFinder_Gernamy_Belgium)
{
  Checkpoints const checkpoints{mercator::FromLatLon(50.76935, 6.42488),
                                mercator::FromLatLon(50.78285, 4.46508)};

  std::set<std::string> const planRegions{
      "Germany_North Rhine-Westphalia_Regierungsbezirk Koln_Aachen", "Belgium_Liege",
      "Belgium_Flemish Brabant"};

  TestRegions(checkpoints, planRegions);
}

// From "Kazakhstan_South" to "Mongolia".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, TestAbsentRegionsFinder_Kazakhstan_Mongolia)
{
  Checkpoints const checkpoints{mercator::FromLatLon(46.12223, 79.28636),
                                mercator::FromLatLon(47.04792, 97.74559)};

  std::set<std::string> const planRegions{"Kazakhstan_South", "China_Xinjiang", "Mongolia"};

  TestRegions(checkpoints, planRegions);
}

// From "Bolivia_North" to "Brazil_North Region_East".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, TestAbsentRegionsFinder_Bolivia_Brazil)
{
  Checkpoints const checkpoints{mercator::FromLatLon(-16.54128, -60.83588),
                                mercator::FromLatLon(-7.38744, -51.29514)};

  std::set<std::string> const planRegions{"Bolivia_North", "Brazil_Mato Grosso",
                                          "Brazil_North Region_East"};

  TestRegions(checkpoints, planRegions);
}

// From "Egypt" to "Sudan_West".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, TestAbsentRegionsFinder_Egypt_Sudan)
{
  Checkpoints const checkpoints{mercator::FromLatLon(25.84571, 30.34731),
                                mercator::FromLatLon(19.82398, 30.20142)};

  std::set<std::string> const planRegions{"Egypt", "Sudan_West"};

  TestRegions(checkpoints, planRegions);
}

// From "Sudan_West" to "Chad".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, TestAbsentRegionsFinder_Sudan_Chad)
{
  Checkpoints const checkpoints{mercator::FromLatLon(12.91113, 25.01158),
                                mercator::FromLatLon(13.44014, 20.23824)};

  std::set<std::string> const planRegions{"Sudan_West", "Chad"};

  TestRegions(checkpoints, planRegions);
}

// From "Australia_Sydney" to "Australia_Victoria".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, TestAbsentRegionsFinder_Sydney_Victoria)
{
  Checkpoints const checkpoints{mercator::FromLatLon(-35.08077, 148.45423),
                                mercator::FromLatLon(-36.81267, 145.74843)};

  std::set<std::string> const planRegions{"Australia_Sydney", "Australia_Victoria"};

  TestRegions(checkpoints, planRegions);
}

// From "Thailand_South" to "Cambodia".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, TestAbsentRegionsFinder_Thailand_Cambodia)
{
  Checkpoints const checkpoints{mercator::FromLatLon(7.89, 98.30),
                                mercator::FromLatLon(11.56, 104.86)};

  std::set<std::string> const planRegions{"Thailand_South", "Cambodia"};

  TestRegions(checkpoints, planRegions);
}

// Inside "China_Sichuan". If the route is inside single mwm we expect empty result from
// RegionsRouter.
UNIT_CLASS_TEST(TestAbsentRegionsFinder, TestAbsentRegionsFinder_China)
{
  Checkpoints const checkpoints{mercator::FromLatLon(30.78611, 102.55829),
                                mercator::FromLatLon(27.54127, 102.02502)};

  std::set<std::string> const planRegions{};

  TestRegions(checkpoints, planRegions);
}

// Inside "Finland_Eastern Finland_North".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, TestAbsentRegionsFinder_Finland)
{
  Checkpoints const checkpoints{mercator::FromLatLon(63.54162, 28.71141),
                                mercator::FromLatLon(64.6790, 28.73029)};

  std::set<std::string> const planRegions{};

  TestRegions(checkpoints, planRegions);
}
}  // namespace
