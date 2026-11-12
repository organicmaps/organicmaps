package app.organicmaps.sdk.location;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.ArgumentMatchers.nullable;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.location.Location;
import android.location.LocationManager;
import android.os.Looper;
import androidx.core.location.LocationListenerCompat;
import androidx.core.location.LocationManagerCompat;
import androidx.core.location.LocationRequestCompat;
import app.organicmaps.sdk.util.log.Logger;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.MockedStatic;

public class AndroidNativeProviderTest
{
  private final LocationManager mManager = mock(LocationManager.class);
  private final BaseLocationProvider.Listener mListener = mock(BaseLocationProvider.Listener.class);
  private MockedStatic<Logger> mLogger;
  private MockedStatic<Looper> mLooper;
  private MockedStatic<LocationManagerCompat> mCompat;
  private AndroidNativeProvider mProvider;

  @Before
  public void setUp()
  {
    mLogger = mockStatic(Logger.class);
    mLooper = mockStatic(Looper.class);
    mCompat = mockStatic(LocationManagerCompat.class);
    final Context context = mock(Context.class);
    when(context.getSystemService(Context.LOCATION_SERVICE)).thenReturn(mManager);
    when(mManager.getProviders(true))
        .thenReturn(List.of(LocationManager.GPS_PROVIDER, LocationManager.NETWORK_PROVIDER));
    mProvider = new AndroidNativeProvider(context, mListener);
  }

  @After
  public void tearDown()
  {
    mCompat.close();
    mLooper.close();
    mLogger.close();
  }

  private LocationListenerCompat lastRequestListener()
  {
    final ArgumentCaptor<LocationListenerCompat> captor = ArgumentCaptor.forClass(LocationListenerCompat.class);
    mCompat.verify(()
                       -> LocationManagerCompat.requestLocationUpdates(eq(mManager), eq(LocationManager.GPS_PROVIDER),
                                                                       any(LocationRequestCompat.class),
                                                                       captor.capture(), nullable(Looper.class)),
                   atLeastOnce());
    return captor.getValue();
  }

  @Test
  public void providerEnabledAfterStopDoesNotBlockRestart()
  {
    mProvider.start(100);
    final LocationListenerCompat stopped = lastRequestListener();
    mProvider.stop();
    stopped.onProviderEnabled(LocationManager.NETWORK_PROVIDER);

    mProvider.start(3000);
    verifyNoInteractions(mListener);
  }

  @Test
  public void stoppedRequestCannotDeliverToARestartedOne()
  {
    mProvider.start(100);
    final LocationListenerCompat stopped = lastRequestListener();
    mProvider.stop();
    mProvider.start(3000);
    final LocationListenerCompat current = lastRequestListener();

    stopped.onLocationChanged(mock(Location.class));
    stopped.onProviderDisabled(LocationManager.GPS_PROVIDER);
    stopped.onProviderDisabled(LocationManager.NETWORK_PROVIDER);
    verifyNoInteractions(mListener);

    final Location location = mock(Location.class);
    current.onLocationChanged(location);
    verify(mListener).onLocationChanged(location);
    verify(mListener, never()).onLocationDisabled();
  }
}
