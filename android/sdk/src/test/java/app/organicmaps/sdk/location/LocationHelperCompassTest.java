package app.organicmaps.sdk.location;

import static org.junit.Assert.assertSame;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.location.Location;
import android.location.LocationManager;
import android.os.Handler;
import android.os.Looper;
import app.organicmaps.sdk.util.log.Logger;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.InOrder;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;

public class LocationHelperCompassTest
{
  private MockedStatic<Logger> mLogger;
  private MockedStatic<Looper> mLooper;
  private MockedConstruction<Handler> mHandlers;
  private MockedConstruction<LocationProviderFactory> mFactories;
  private final SensorHelper mSensors = mock(SensorHelper.class);
  private LocationHelper mHelper;

  @Before
  public void setUp()
  {
    mLogger = mockStatic(Logger.class);
    mLooper = mockStatic(Looper.class);
    mHandlers = mockConstruction(Handler.class);
    final BaseLocationProvider provider = mock(BaseLocationProvider.class);
    mFactories =
        mockConstruction(LocationProviderFactory.class,
                         (factory, construction) -> when(factory.getProvider(any(), any())).thenReturn(provider));
    mHelper = spy(new LocationHelper(mock(Context.class), mSensors));
    doReturn(true).when(mHelper).isActive();
    // Exercise location acceptance without starting providers or calling location JNI.
    mHelper.onEnteredIntoFirstRun();
  }

  @After
  public void tearDown()
  {
    mFactories.close();
    mHandlers.close();
    mLooper.close();
    mLogger.close();
  }

  @Test
  public void rejectedLocationsDoNotUpdateTheCompass()
  {
    final Location accepted = location(LocationManager.GPS_PROVIDER, 10f, 10_000_000_000L);
    doReturn(false).when(mHelper).isActive();
    mHelper.onLocationChanged(accepted);
    doReturn(true).when(mHelper).isActive();
    mHelper.onLocationChanged(location(LocationManager.NETWORK_PROVIDER, 0f, 10_000_000_000L));
    verifyNoInteractions(mSensors);

    mHelper.onLocationChanged(accepted);
    verify(mSensors).updateLocation(accepted);
    clearInvocations(mSensors);
    mHelper.onLocationChanged(location(LocationManager.GPS_PROVIDER, 1f, 9_000_000_000L));
    mHelper.onLocationChanged(location(LocationManager.NETWORK_PROVIDER, 100f, 11_000_000_000L));
    assertSame(accepted, mHelper.getSavedLocation());
    verifyNoInteractions(mSensors);
  }

  @Test
  public void acceptedPositionIsSavedBeforeCompassAndLocationNotifications()
  {
    final LocationListener locations = mock(LocationListener.class);
    mHelper.addListener(locations);
    final Location accepted = location(LocationManager.GPS_PROVIDER, 10f, 10_000_000_000L);
    doAnswer(call -> {
      assertSame(accepted, mHelper.getSavedLocation());
      return null;
    })
        .when(mSensors)
        .updateLocation(accepted);

    mHelper.onLocationChanged(accepted);

    final InOrder order = inOrder(mSensors, locations);
    order.verify(mSensors).updateLocation(accepted);
    order.verify(locations).onLocationUpdated(accepted);
  }

  private static Location location(String provider, float accuracy, long elapsedNanos)
  {
    final Location location = mock(Location.class);
    when(location.getProvider()).thenReturn(provider);
    when(location.getAccuracy()).thenReturn(accuracy);
    when(location.getElapsedRealtimeNanos()).thenReturn(elapsedNanos);
    return location;
  }
}
