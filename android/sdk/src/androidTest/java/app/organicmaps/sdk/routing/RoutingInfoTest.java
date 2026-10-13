package app.organicmaps.sdk.routing;

import static org.junit.Assert.assertEquals;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class RoutingInfoTest
{
  private static final int[] CASES = {
      0, 29, 59, 89, 90, 269, 270, 1649, 1650, 1665, 2085, 3569, 3570, 3630, 86400, 94485, Integer.MAX_VALUE};

  @BeforeClass
  public static void loadCore()
  {
    System.loadLibrary("organicmaps");
  }

  @Test
  public void followingInfoPreservesRawSecondsAtTheJniBoundary()
  {
    for (int seconds : CASES)
    {
      for (int flags = 0; flags < 4; ++flags)
      {
        boolean exceeded = (flags & 1) != 0;
        boolean warning = (flags & 2) != 0;
        RoutingInfo info = nativeCreateFollowingInfo(seconds, exceeded, warning);
        assertEquals(seconds + " seconds", seconds, info.totalTimeInSeconds);
        assertEquals(exceeded, info.isSpeedCamLimitExceeded());
        assertEquals(warning, info.shouldPlayWarningSignal());
        assertEquals(100.0, info.distToTarget.mDistance, 0.0);
      }
    }
  }

  @Test
  public void transitInfoPreservesRawSecondsAtTheJniBoundary()
  {
    for (int seconds : CASES)
    {
      TransitRouteInfo info = nativeCreateTransitInfo(seconds);
      assertEquals(seconds + " seconds", seconds, info.getTotalTime());
      assertEquals(21, info.getTotalPedestrianTimeInSec());
      assertEquals(21, info.getTransitSteps().get(0).getTimeInSec());
    }
  }

  private static native RoutingInfo nativeCreateFollowingInfo(int seconds, boolean speedCamLimitExceeded,
                                                              boolean shouldPlayWarningSignal);

  private static native TransitRouteInfo nativeCreateTransitInfo(int seconds);
}
