package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.mockStatic;

import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.settings.RoadType;
import java.util.EnumSet;
import org.junit.Test;
import org.mockito.MockedStatic;

public class RoutingOptionsMaskTest
{
  @Test
  public void queriesAndBadgeCountsAgreeForEveryCombination()
  {
    try (MockedStatic<RoutingOptions> options = mockStatic(RoutingOptions.class, CALLS_REAL_METHODS))
    {
      for (Router router : Router.values())
      {
        for (int mask = 0; mask < 32; ++mask)
        {
          final int stored = mask;
          options.when(() -> RoutingOptions.getOptions(router)).thenReturn(stored);
          EnumSet<RoadType> expected = EnumSet.noneOf(RoadType.class);
          for (RoadType road : RoadType.values())
          {
            boolean enabled = (stored & road.nativeValue) != 0;
            if (enabled)
              expected.add(road);
            assertEquals(enabled, RoutingOptions.hasOption(router, road));
          }
          assertEquals(expected.size(), Integer.bitCount(RoutingOptions.getOptions(router)));
          assertEquals(!expected.isEmpty(), RoutingOptions.hasAnyOptions(router));
        }
      }
    }
  }
}
