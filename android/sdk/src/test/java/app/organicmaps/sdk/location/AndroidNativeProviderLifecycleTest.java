package app.organicmaps.sdk.location;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

import android.content.Context;
import android.location.Location;
import android.location.LocationManager;
import android.os.Looper;
import app.organicmaps.sdk.util.log.Logger;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(manifest = Config.NONE, sdk = 28)
public class AndroidNativeProviderLifecycleTest
{
  @Test
  public void queuedUnavailableReportCannotDisableARestartedRequest()
  {
    try (var logger = mockStatic(Logger.class))
    {
      final Context context = RuntimeEnvironment.getApplication();
      final LocationManager manager = (LocationManager) context.getSystemService(Context.LOCATION_SERVICE);
      final var shadowManager = Shadows.shadowOf(manager);
      shadowManager.setProviderEnabled(LocationManager.GPS_PROVIDER, false);
      shadowManager.setProviderEnabled(LocationManager.NETWORK_PROVIDER, false);
      final BaseLocationProvider.Listener listener = mock(BaseLocationProvider.Listener.class);
      final AndroidNativeProvider provider = new AndroidNativeProvider(context, listener);
      provider.start(100);
      provider.stop();
      shadowManager.setProviderEnabled(LocationManager.GPS_PROVIDER, true);
      provider.start(3000);
      Shadows.shadowOf(Looper.getMainLooper()).idle();

      verify(listener, never()).onLocationDisabled();
      final Location location = new Location(LocationManager.GPS_PROVIDER);
      shadowManager.simulateLocation(location);
      Shadows.shadowOf(Looper.getMainLooper()).idle();
      verify(listener).onLocationChanged(any(Location.class));
      provider.stop();
    }
  }
}
