package app.organicmaps.sdk.location;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertSame;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyDouble;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.hardware.GeomagneticField;
import android.hardware.SensorEvent;
import android.hardware.SensorManager;
import android.location.Location;
import android.location.LocationManager;
import android.os.Handler;
import android.os.Looper;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.NetworkPolicy;
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
  private MockedStatic<NetworkPolicy> mNetwork;
  private MockedStatic<RoutingController> mRouting;
  private MockedStatic<LocationUtils> mLocationUtils;
  private MockedConstruction<Handler> mHandlers;
  private MockedConstruction<LocationProviderFactory> mFactories;
  private MockedConstruction<GeomagneticField> mFields;
  private LocationHelper mHelper;
  private SensorHelper mSensorHelper;
  private BaseLocationProvider.Listener mProviderListener;

  @Before
  public void setUp()
  {
    mLogger = mockStatic(Logger.class);
    mLooper = mockStatic(Looper.class);
    mHandlers = mockConstruction(Handler.class);
    mNetwork = mockStatic(NetworkPolicy.class);
    mRouting = mockStatic(RoutingController.class);
    final RoutingController routing = mock(RoutingController.class);
    when(routing.isNavigating()).thenReturn(true);
    mRouting.when(RoutingController::get).thenReturn(routing);
    mLocationUtils = mockStatic(LocationUtils.class, CALLS_REAL_METHODS);
    final Context context = mock(Context.class);
    mLocationUtils.when(() -> LocationUtils.checkFineLocationPermission(context)).thenReturn(false);
    final BaseLocationProvider provider = mock(BaseLocationProvider.class);
    mFactories =
        mockConstruction(LocationProviderFactory.class,
                         (factory, construction) -> when(factory.getProvider(any(), any())).thenAnswer(call -> {
                           mProviderListener = call.getArgument(1);
                           return provider;
                         }));
    mFields =
        mockConstruction(GeomagneticField.class, (field, construction) -> when(field.getDeclination()).thenReturn(20f));
    when(context.getSystemService(Context.SENSOR_SERVICE)).thenReturn(mock(SensorManager.class));
    mSensorHelper = new SensorHelper(context);
    mHelper = new LocationHelper(context, mSensorHelper);
    // Keep the real notification path while avoiding unrelated location JNI.
    mHelper.onEnteredIntoFirstRun();
  }

  @After
  public void tearDown()
  {
    mFields.close();
    mFactories.close();
    mLocationUtils.close();
    mRouting.close();
    mNetwork.close();
    mHandlers.close();
    mLooper.close();
    mLogger.close();
  }

  @Test
  public void rejectedProviderFixesDoNotChangeTheMagneticModel()
  {
    final Location accepted = location(LocationManager.GPS_PROVIDER, 10f, 10_000_000_000L);
    mProviderListener.onLocationChanged(accepted);
    assertEquals(0, mFields.constructed().size());

    mHelper.start();
    mProviderListener.onLocationChanged(location(LocationManager.NETWORK_PROVIDER, 0f, 10_000_000_000L));
    assertEquals(0, mFields.constructed().size());
    mProviderListener.onLocationChanged(accepted);
    assertSame(accepted, mHelper.getSavedLocation());
    assertEquals(1, mFields.constructed().size());

    final LocationListener listener = mock(LocationListener.class);
    mHelper.addListener(listener);
    clearInvocations(listener);
    mProviderListener.onLocationChanged(location(LocationManager.GPS_PROVIDER, 1f, 9_000_000_000L));
    mProviderListener.onLocationChanged(location(LocationManager.NETWORK_PROVIDER, 100f, 11_000_000_000L));
    assertSame(accepted, mHelper.getSavedLocation());
    assertEquals(1, mFields.constructed().size());
    verifyNoInteractions(listener);
  }

  @Test
  public void compassCallbackSeesTheAcceptedPositionBeforeLocationObservers()
  {
    mHelper.start();
    try (MockedStatic<SensorManager> sensors = mockStatic(SensorManager.class))
    {
      sensors.when(() -> SensorManager.getOrientation(any(float[].class), any(float[].class)))
          .thenAnswer(call -> call.getArgument(1));
      final SensorEvent event = mock(SensorEvent.class);
      event.accuracy = SensorManager.SENSOR_STATUS_ACCURACY_HIGH;
      mSensorHelper.onSensorChanged(event);
    }

    final SensorListener compass = mock(SensorListener.class);
    mSensorHelper.addListener(compass);
    clearInvocations(compass);
    final LocationListener locations = mock(LocationListener.class);
    mHelper.addListener(locations);
    final Location accepted = location(LocationManager.GPS_PROVIDER, 10f, 10_000_000_000L);
    doAnswer(call -> {
      assertSame(accepted, mHelper.getSavedLocation());
      return null;
    })
        .when(compass)
        .onCompassUpdated(anyDouble());

    mProviderListener.onLocationChanged(accepted);

    assertEquals(Math.toRadians(20), mSensorHelper.getSavedNorth(), 1e-6);
    final InOrder order = inOrder(compass, locations);
    order.verify(compass).onCompassUpdated(mSensorHelper.getSavedNorth());
    order.verify(locations).onLocationUpdated(accepted);
    assertEquals(1, mFields.constructed().size());
  }

  private static Location location(String provider, float accuracy, long elapsedNanos)
  {
    final Location location = mock(Location.class);
    when(location.getProvider()).thenReturn(provider);
    when(location.getAccuracy()).thenReturn(accuracy);
    when(location.getElapsedRealtimeNanos()).thenReturn(elapsedNanos);
    when(location.getLatitude()).thenReturn(47.5);
    when(location.getLongitude()).thenReturn(8.5);
    return location;
  }
}
