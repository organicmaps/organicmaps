#include "testing/testing.hpp"

#include "indexer/map_style.hpp"

#include <optional>

namespace map_style_tests
{
UNIT_TEST(GetMapStyleForFamily)
{
  TEST_EQUAL(GetMapStyleForFamily(MapStyleFamily::Default, false /* dark */), MapStyleDefaultLight, ());
  TEST_EQUAL(GetMapStyleForFamily(MapStyleFamily::Default, true), MapStyleDefaultDark, ());

  TEST_EQUAL(GetMapStyleForFamily(MapStyleFamily::Vehicle, false), MapStyleVehicleLight, ());
  TEST_EQUAL(GetMapStyleForFamily(MapStyleFamily::Vehicle, true), MapStyleVehicleDark, ());

  TEST_EQUAL(GetMapStyleForFamily(MapStyleFamily::Outdoors, false), MapStyleOutdoorsLight, ());
  TEST_EQUAL(GetMapStyleForFamily(MapStyleFamily::Outdoors, true), MapStyleOutdoorsDark, ());
}

UNIT_TEST(SelectMapStyleFamily)
{
  // Priority, highest first: Vehicle (while following) > Outdoors (layer flag) > Default.
  TEST_EQUAL(SelectMapStyleFamily(true /* vehicleFollowing */, false /* outdoors */), MapStyleFamily::Vehicle, ());
  TEST_EQUAL(SelectMapStyleFamily(true, true), MapStyleFamily::Vehicle, ());  // Vehicle outranks Outdoors.
  TEST_EQUAL(SelectMapStyleFamily(false, true), MapStyleFamily::Outdoors, ());
  TEST_EQUAL(SelectMapStyleFamily(false, false), MapStyleFamily::Default, ());
}

UNIT_TEST(NormalizeStartupMapStyle)
{
  // Vehicle is transient: with no active route at launch it collapses to the base family at the same
  // darkness. Flag absent + non-Outdoors style => Default, nothing to persist.
  {
    auto const s = NormalizeStartupMapStyle(MapStyleVehicleDark, std::nullopt);
    TEST_EQUAL(s.style, MapStyleDefaultDark, ());
    TEST(!s.outdoorsEnabled, ());
    TEST(!s.persistOutdoorsFlag, ());
  }
  // Flag explicitly true collapses Vehicle to the Outdoors base; nothing to persist (already stored).
  {
    auto const s = NormalizeStartupMapStyle(MapStyleVehicleLight, std::optional<bool>(true));
    TEST_EQUAL(s.style, MapStyleOutdoorsLight, ());
    TEST(s.outdoorsEnabled, ());
    TEST(!s.persistOutdoorsFlag, ());
  }
  // Legacy iOS persisted only the Outdoors style (flag absent): derive the flag and persist it.
  {
    auto const s = NormalizeStartupMapStyle(MapStyleOutdoorsDark, std::nullopt);
    TEST_EQUAL(s.style, MapStyleOutdoorsDark, ());
    TEST(s.outdoorsEnabled, ());
    TEST(s.persistOutdoorsFlag, ());
  }
  // Explicit false is authoritative even under an inconsistent Outdoors style (e.g. a crash after
  // saving false but before applying Default): collapse to Default, do not re-enable Outdoors.
  {
    auto const s = NormalizeStartupMapStyle(MapStyleOutdoorsLight, std::optional<bool>(false));
    TEST_EQUAL(s.style, MapStyleDefaultLight, ());
    TEST(!s.outdoorsEnabled, ());
    TEST(!s.persistOutdoorsFlag, ());
  }
  // Default style + explicit outdoors flag => Outdoors (the previously-unhandled legacy Qt state).
  {
    auto const s = NormalizeStartupMapStyle(MapStyleDefaultDark, std::optional<bool>(true));
    TEST_EQUAL(s.style, MapStyleOutdoorsDark, ());
    TEST(s.outdoorsEnabled, ());
    TEST(!s.persistOutdoorsFlag, ());
  }
}
}  // namespace map_style_tests
