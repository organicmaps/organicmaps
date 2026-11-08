package app.organicmaps;

import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.util.StringUtils;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.Resetter;

public final class NavigationNativeShadows
{
  @Implements(value = StringUtils.class, isInAndroidSdk = false)
  public static class SpeedFormatting
  {
    @Implementation
    public static int nativeFormatSpeed(double speed)
    {
      return (int) Math.round(speed * 3.6);
    }

    @Implementation
    public static String nativeGetLocalizedSpeedUnits()
    {
      return "km/h";
    }
  }

  @Implements(value = Framework.class, isInAndroidSdk = false)
  public static class Routing
  {
    public static String[] notifications;
    public static boolean finished;
    public static int notificationRequests;

    @Implementation
    public static String[] nativeGenerateNotifications(boolean announceStreets)
    {
      ++notificationRequests;
      return notifications;
    }

    @Implementation
    public static boolean nativeIsRouteFinished()
    {
      return finished;
    }

    @Resetter
    public static void reset()
    {
      notifications = null;
      finished = false;
      notificationRequests = 0;
    }
  }
}
