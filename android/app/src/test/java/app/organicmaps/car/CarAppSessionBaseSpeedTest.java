package app.organicmaps.car;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.location.Location;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.routing.RoutingController;
import java.lang.reflect.Method;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, manifest = Config.NONE, sdk = 33,
        shadows = CarAppSessionBaseSpeedTest.NativeSignStyle.class)
public class CarAppSessionBaseSpeedTest
{
  @Test
  public void firstFixDuringNavigationInitializesSignStyleButNotRawSpeed() throws ReflectiveOperationException
  {
    NativeSignStyle.requests = 0;
    final RoutingController controller = mock(RoutingController.class);
    when(controller.isNavigating()).thenReturn(true);
    final CarAppSessionBase session = mock(CarAppSessionBase.class);
    session.mSurfaceRenderer = mock(Renderer.class);
    final Method update = CarAppSessionBase.class.getDeclaredMethod("updateSpeed", Location.class);
    update.setAccessible(true);
    try (MockedStatic<RoutingController> routing = mockStatic(RoutingController.class))
    {
      routing.when(RoutingController::get).thenReturn(controller);
      update.invoke(session, (Location) null);
      verify(session.mSurfaceRenderer, never()).setSpeedLimitSignType(anyInt());

      final Location first = new Location("gps");
      first.setLatitude(47.60);
      first.setLongitude(-122.33);
      first.setSpeed(20);
      update.invoke(session, first);
      verify(session.mSurfaceRenderer).setSpeedLimitSignType(Framework.SpeedLimitSignType.MUTCD);
      assertEquals(47.60, NativeSignStyle.latitude, 0);
      assertEquals(-122.33, NativeSignStyle.longitude, 0);

      final Location later = new Location(first);
      later.setLatitude(48.0);
      update.invoke(session, later);
      assertEquals(1, NativeSignStyle.requests);
      verify(session.mSurfaceRenderer, never()).setCurrentSpeed(anyInt(), anyString());
    }
  }

  @Implements(value = Framework.class, isInAndroidSdk = false)
  public static class NativeSignStyle
  {
    static int requests;
    static double latitude;
    static double longitude;

    @Implementation
    public static int nativeGetSpeedLimitSignType(double lat, double lon)
    {
      ++requests;
      latitude = lat;
      longitude = lon;
      return Framework.SpeedLimitSignType.MUTCD;
    }
  }
}
