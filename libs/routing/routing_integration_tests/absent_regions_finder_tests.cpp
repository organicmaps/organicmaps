#include "testing/testing.hpp"

#include "map/framework.hpp"
#include "map/routing_manager.hpp"

#include "storage/routing_helpers.hpp"
#include "storage/storage.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/mercator.hpp"

#include <memory>
#include <set>
#include <string>

namespace absent_regions_finder_tests
{
using namespace routing;

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
  m_numMwmIds = CreateNumMwmIds(m_framework.GetStorage());

  m_countryFileGetter = [this](m2::PointD const & p) -> std::string
  { return m_callbacks.m_countryInfoGetter().GetRegionCountryId(p); };

  m_localFileChecker = [&](std::string const & countryFile)
  {
    MwmSet::MwmId const mwmId =
        m_callbacks.m_dataSourceGetter().GetMwmIdByCountryFile(platform::CountryFile(countryFile));
    return mwmId.IsAlive();
  };
}

void TestAbsentRegionsFinder::TestRegions(Checkpoints const & checkpoints, std::set<std::string> const & planRegions)
{
  std::set<std::string> const & factRegions = GetRegions(checkpoints);
  TEST_EQUAL(planRegions, factRegions, ());
}

std::set<std::string> TestAbsentRegionsFinder::GetRegions(Checkpoints const & checkpoints)
{
  AbsentRegionsFinder finder(m_countryFileGetter, m_localFileChecker, m_numMwmIds, m_callbacks.m_dataSourceGetter());
  RouterDelegate delegate;

  finder.GenerateAbsentRegions(checkpoints, delegate);

  std::set<std::string> regions;
  finder.GetAllRegions(regions);

  return regions;
}

// From "Russia_Republic of Karelia_South" to "Russia_Krasnodar Krai".
// https://www.openstreetmap.org/directions?engine=fossgis_osrm_car&route=61.759%2C34.452%3B45.070%2C38.940#map=5/54.869/40.210
/**
 * @todo Current test set and Organic set differ from OSRM route. Need to make deep investigation here.
 * OSRM wants Novgorod, Tver, Moscow (looks good).
 * Organic wants Vologda, Tver, Moscow East, Ryazan (also may be good).
 * Current test set doesn't have Tver (obvious error).
 * Ukraine_Luhansk Oblast is not a good idea for both variants.
 */
UNIT_CLASS_TEST(TestAbsentRegionsFinder, Karelia_Krasnodar)
{
  Checkpoints const checkpoints{mercator::FromLatLon(61.76, 34.45), mercator::FromLatLon(45.07, 38.94)};

  // Current test set.
  /*
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
  */

  // Organic test set.
  std::set<std::string> const planRegions{"Russia_Krasnodar Krai",
                                          "Russia_Leningradskaya Oblast_Southeast",
                                          "Russia_Lipetsk Oblast",
                                          "Russia_Moscow Oblast_East",
                                          "Russia_Republic of Karelia_South",
                                          "Russia_Rostov Oblast",
                                          "Russia_Ryazan Oblast",
                                          "Russia_Tula Oblast",
                                          "Russia_Tver Oblast",
                                          "Russia_Vologda Oblast",
                                          "Russia_Voronezh Oblast",
                                          "Ukraine_Luhansk Oblast"};

  TestRegions(checkpoints, planRegions);
}

// From "Canada_Ontario_Kingston" to "US_Maryland_and_DC".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, Kingston_DC)
{
  Checkpoints const checkpoints{mercator::FromLatLon(45.38, -75.69), mercator::FromLatLon(38.91, -77.031)};

  std::set<std::string> const planRegions{"Canada_Ontario_Kingston", "US_Maryland_Baltimore", "US_Maryland_and_DC",
                                          "US_New York_North",       "US_New York_West",      "US_Pennsylvania_Central",
                                          "US_Pennsylvania_Scranton"};

  TestRegions(checkpoints, planRegions);
}

// From "US_Colorado_Aspen" to "Canada_Saskatchewan_Saskatoon".
// https://www.openstreetmap.org/directions?engine=fossgis_osrm_car&route=39.95763%2C-106.79994%3B49.92034%2C-106.99302
UNIT_CLASS_TEST(TestAbsentRegionsFinder, Colorado_Saskatchewan)
{
  Checkpoints const checkpoints{mercator::FromLatLon(39.95763, -106.79994), mercator::FromLatLon(49.92034, -106.99302)};

  std::set<std::string> const planRegions{"Canada_Saskatchewan_Saskatoon", "US_Colorado_Aspen", "US_Montana_East",
                                          "US_Wyoming"};

  TestRegions(checkpoints, planRegions);
}

// From "Belgium_Flemish Brabant" to "Germany_North Rhine-Westphalia_Regierungsbezirk Koln_Aachen".
// https://www.openstreetmap.org/directions?engine=fossgis_osrm_car&route=50.87763%2C4.44676%3B50.76935%2C6.42488
UNIT_CLASS_TEST(TestAbsentRegionsFinder, Belgium_Germany)
{
  Checkpoints const checkpoints{mercator::FromLatLon(50.87763, 4.44676), mercator::FromLatLon(50.76935, 6.42488)};

  // OSRM, Valhalla with E40.
  std::set<std::string> const expected1 = {
      "Belgium_Flemish Brabant",
      "Belgium_Walloon Brabant",
      "Belgium_Liege",
      "Germany_North Rhine-Westphalia_Regierungsbezirk Koln_Aachen",
  };

  // GraphHopper with E314.
  std::set<std::string> const expected2 = {
      "Belgium_Flemish Brabant",
      "Belgium_Limburg",
      "Germany_North Rhine-Westphalia_Regierungsbezirk Koln_Aachen",
      "Netherlands_Limburg",
  };

  auto const actual = GetRegions(checkpoints);
  TEST(actual == expected1 || actual == expected2, (actual));
}

// From "Germany_North Rhine-Westphalia_Regierungsbezirk Koln_Aachen" to "Belgium_Flemish Brabant".
// https://www.openstreetmap.org/directions?engine=fossgis_osrm_car&route=50.76935%2C6.42488%3B50.78285%2C4.46508
UNIT_CLASS_TEST(TestAbsentRegionsFinder, Germany_Belgium)
{
  Checkpoints const checkpoints{mercator::FromLatLon(50.76935, 6.42488), mercator::FromLatLon(50.78285, 4.46508)};

  // Valhalla with E40.
  std::set<std::string> const expected1 = {"Belgium_Flemish Brabant", "Belgium_Walloon Brabant", "Belgium_Liege",
                                           "Belgium_Limburg",
                                           "Germany_North Rhine-Westphalia_Regierungsbezirk Koln_Aachen"};

  // OSRM, GraphHopper with E314.
  std::set<std::string> const expected2 = {
      "Belgium_Flemish Brabant",
      "Belgium_Limburg",
      "Germany_North Rhine-Westphalia_Regierungsbezirk Koln_Aachen",
      "Netherlands_Limburg",
  };

  auto const actual = GetRegions(checkpoints);
  TEST(actual == expected1 || actual == expected2, (actual));
}

// From "Kazakhstan_South" to "Mongolia".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, Kazakhstan_Mongolia)
{
  Checkpoints const checkpoints{mercator::FromLatLon(46.12223, 79.28636), mercator::FromLatLon(47.04792, 97.74559)};

  std::set<std::string> const planRegions{"Kazakhstan_South", "China_Xinjiang", "Mongolia"};

  TestRegions(checkpoints, planRegions);
}

// From "Bolivia_North" to "Brazil_North Region_East".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, Bolivia_Brazil)
{
  Checkpoints const checkpoints{mercator::FromLatLon(-16.54128, -60.83588), mercator::FromLatLon(-7.38744, -51.29514)};

  std::set<std::string> const planRegions{"Bolivia_North", "Brazil_Mato Grosso", "Brazil_North Region_East"};

  TestRegions(checkpoints, planRegions);
}

// From "Egypt" to "Sudan_West".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, Egypt_Sudan)
{
  Checkpoints const checkpoints{mercator::FromLatLon(25.84571, 30.34731), mercator::FromLatLon(19.82398, 30.20142)};

  std::set<std::string> const planRegions{"Egypt", "Sudan_West"};

  TestRegions(checkpoints, planRegions);
}

// From "Sudan_West" to "Chad".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, Sudan_Chad)
{
  Checkpoints const checkpoints{mercator::FromLatLon(12.91113, 25.01158), mercator::FromLatLon(13.44014, 20.23824)};

  std::set<std::string> const planRegions{"Sudan_West", "Chad"};

  TestRegions(checkpoints, planRegions);
}

// From "Australia_Sydney" to "Australia_Victoria".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, Sydney_Victoria)
{
  Checkpoints const checkpoints{mercator::FromLatLon(-35.08077, 148.45423), mercator::FromLatLon(-36.81267, 145.74843)};

  std::set<std::string> const planRegions{"Australia_Sydney", "Australia_Victoria"};

  TestRegions(checkpoints, planRegions);
}

// From "Thailand_South" to "Cambodia".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, Thailand_Cambodia)
{
  Checkpoints const checkpoints{mercator::FromLatLon(7.89, 98.30), mercator::FromLatLon(11.56, 104.86)};

  std::set<std::string> const planRegions{"Thailand_South", "Cambodia"};

  TestRegions(checkpoints, planRegions);
}

// Inside "China_Sichuan". If the route is inside single mwm we expect empty result from
// RegionsRouter.
UNIT_CLASS_TEST(TestAbsentRegionsFinder, China)
{
  Checkpoints const checkpoints{mercator::FromLatLon(30.78611, 102.55829), mercator::FromLatLon(27.54127, 102.02502)};

  TestRegions(checkpoints, {});
}

// Inside "Finland_Eastern Finland_North".
UNIT_CLASS_TEST(TestAbsentRegionsFinder, Finland)
{
  Checkpoints const checkpoints{mercator::FromLatLon(63.54162, 28.71141), mercator::FromLatLon(64.6790, 28.73029)};

  TestRegions(checkpoints, {});
}

// https://github.com/organicmaps/organicmaps/issues/980
UNIT_CLASS_TEST(TestAbsentRegionsFinder, BC_Alberta)
{
  Checkpoints const checkpoints{mercator::FromLatLon(49.2608724, -123.1139529),
                                mercator::FromLatLon(53.5354110, -113.5079960)};

  std::set<std::string> const planRegions{"Canada_Alberta_Edmonton", "Canada_Alberta_South",
                                          "Canada_British Columbia_Southeast", "Canada_British Columbia_Vancouver"};

  TestRegions(checkpoints, planRegions);
}

// https://github.com/organicmaps/organicmaps/issues/1721
UNIT_CLASS_TEST(TestAbsentRegionsFinder, Germany_Cologne_Croatia_Zagreb)
{
  Checkpoints const checkpoints{mercator::FromLatLon(50.924, 6.943), mercator::FromLatLon(45.806, 15.963)};

  /// @todo Optimal route should include Graz-Maribor-Zagreb.
  auto const & rgns = GetRegions(checkpoints);
  TEST(rgns.count("Austria_Styria_Graz") > 0, ());
}

UNIT_CLASS_TEST(TestAbsentRegionsFinder, Russia_SPB_Pechory)
{
  Checkpoints const checkpoints{mercator::FromLatLon(59.9387323, 30.3162295),
                                mercator::FromLatLon(57.8133044, 27.6081855)};

  /// @todo Optimal should not include Estonia.
  for (auto const & rgn : GetRegions(checkpoints))
    TEST(!rgn.starts_with("Estonia"), ());
}

}  // namespace absent_regions_finder_tests
