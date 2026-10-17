#include "testing/testing.hpp"

#include "map/framework.hpp"

#include "indexer/map_style.hpp"

namespace map_style_resolve_tests
{
// Framework wiring for ResolveMapStyleForMode(): it feeds the live night mode and outdoors flag into
// the pure SelectMapStyleFamily/GetMapStyleForFamily (unit-tested in indexer_tests). With no active
// route it picks the base family at the stored darkness. The Vehicle-while-following branch needs a
// calculated route and is not exercised here.
UNIT_TEST(Framework_ResolveMapStyleForMode_BaseFamilies)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  TEST(!framework.GetRoutingManager().IsRoutingFollowing(), ("No active route at construction"));

  Framework::SaveOutdoorsEnabled(false);
  framework.SetNightMode(false);
  TEST_EQUAL(framework.ResolveMapStyleForMode(), MapStyleDefaultLight, ());
  framework.SetNightMode(true);
  TEST_EQUAL(framework.ResolveMapStyleForMode(), MapStyleDefaultDark, ());

  Framework::SaveOutdoorsEnabled(true);
  framework.SetNightMode(false);
  TEST_EQUAL(framework.ResolveMapStyleForMode(), MapStyleOutdoorsLight, ());
  framework.SetNightMode(true);
  TEST_EQUAL(framework.ResolveMapStyleForMode(), MapStyleOutdoorsDark, ());

  Framework::SaveOutdoorsEnabled(false);  // Restore the global default for other tests.
}

// A direct style change (debug commands, SDK MapStyle.set/mark) must keep m_nightMode in sync, so a
// subsequent routing/outdoors resolve does not reverse the user's darkness.
UNIT_TEST(Framework_MarkMapStyleSyncsNightMode)
{
  Framework framework(FrameworkParams(false /* m_enableDiffs */));
  Framework::SaveOutdoorsEnabled(false);

  framework.SetNightMode(false);
  framework.MarkMapStyle(MapStyleDefaultDark);  // Direct dark style while the night mode was light.
  TEST_EQUAL(framework.ResolveMapStyleForMode(), MapStyleDefaultDark, ("resolve must keep the darkness"));

  framework.MarkMapStyle(MapStyleDefaultLight);
  TEST_EQUAL(framework.ResolveMapStyleForMode(), MapStyleDefaultLight, ());

  framework.MarkMapStyle(kDefaultMapStyle);  // Restore for other tests.
}
}  // namespace map_style_resolve_tests
