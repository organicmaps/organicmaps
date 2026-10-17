package app.organicmaps.util;

import static org.junit.Assert.assertEquals;

import android.content.pm.ActivityInfo;
import org.junit.Test;

public class ScreenOrientationTest
{
  @Test
  public void uprightScreensTurnOnlyWhenAllowed()
  {
    assertEquals(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT,
                 Utils.getScreenOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT, false));
    assertEquals(ActivityInfo.SCREEN_ORIENTATION_FULL_USER,
                 Utils.getScreenOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT, true));
    // Allowed and forbidden again while the activity lives.
    assertEquals(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT,
                 Utils.getScreenOrientation(ActivityInfo.SCREEN_ORIENTATION_FULL_USER, false));
  }

  @Test
  public void otherScreensKeepTheirOrientation()
  {
    // E.g. the ones of Android Auto.
    for (boolean allowed : new boolean[] {false, true})
    {
      assertEquals(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED,
                   Utils.getScreenOrientation(ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED, allowed));
      assertEquals(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE,
                   Utils.getScreenOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE, allowed));
    }
  }
}
