package app.organicmaps.sdk.car;

import static org.junit.Assert.assertEquals;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.hardware.GeomagneticField;
import android.hardware.SensorEvent;
import android.hardware.SensorManager;
import android.location.Location;
import android.view.Surface;
import androidx.car.app.CarContext;
import androidx.car.app.hardware.CarHardwareManager;
import androidx.car.app.hardware.common.CarValue;
import androidx.car.app.hardware.common.OnCarDataAvailableListener;
import androidx.car.app.hardware.info.CarSensors;
import androidx.car.app.hardware.info.Compass;
import androidx.core.content.ContextCompat;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.SensorHelper;
import app.organicmaps.sdk.util.log.Logger;
import java.util.List;
import java.util.concurrent.Executor;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;

public class CarSensorsManagerTest
{
  private MockedStatic<Logger> mLogger;
  private MockedStatic<Map> mMap;
  private MockedStatic<ContextCompat> mContextCompat;
  private MockedStatic<SensorManager> mSensorApi;
  private MockedConstruction<GeomagneticField> mFields;
  private final CarSensors mCarSensors = mock(CarSensors.class);
  private CarSensorsManager mManager;
  private SensorHelper mSensorHelper;
  private OnCarDataAvailableListener<Compass> mCarCompassListener;
  private float mDeclinationDegrees = 20;
  private float mYawRadians;

  @Before
  public void setUp()
  {
    mLogger = mockStatic(Logger.class);
    mMap = mockStatic(Map.class);
    mContextCompat = mockStatic(ContextCompat.class);
    mSensorApi = mockStatic(SensorManager.class);
    mSensorApi.when(() -> SensorManager.getOrientation(any(float[].class), any(float[].class))).thenAnswer(call -> {
      final float[] orientation = call.getArgument(1);
      orientation[0] = mYawRadians;
      return orientation;
    });
    mFields = mockConstruction(GeomagneticField.class,
                               (field, construction) -> when(field.getDeclination()).thenReturn(mDeclinationDegrees));
    final CarContext context = mock(CarContext.class);
    final CarHardwareManager hardware = mock(CarHardwareManager.class);
    when(context.getCarService(CarHardwareManager.class)).thenReturn(hardware);
    when(hardware.getCarSensors()).thenReturn(mCarSensors);
    when(context.getSystemService(Context.SENSOR_SERVICE)).thenReturn(mock(SensorManager.class));
    mContextCompat.<Executor>when(() -> ContextCompat.getMainExecutor(context)).thenReturn(Runnable::run);
    doAnswer(call -> {
      mCarCompassListener = call.getArgument(2);
      return null;
    })
        .when(mCarSensors)
        .addCompassListener(anyInt(), any(), any());
    final LocationHelper locations = mock(LocationHelper.class);
    when(locations.isActive()).thenReturn(true);
    mSensorHelper = new SensorHelper(context);
    mSensorHelper.updateLocation(mock(Location.class));
    sample(30);
    mSensorHelper.setRotation(Surface.ROTATION_90);
    mManager = new CarSensorsManager(context, mSensorHelper, locations);
    mManager.onStart();
  }

  @After
  public void tearDown()
  {
    mFields.close();
    mSensorApi.close();
    mContextCompat.close();
    mMap.close();
    mLogger.close();
  }

  @Test
  public void carHeadingGetsDeclinationOnceWithoutPhoneRotationOrChangingThePhoneSample()
  {
    mCarCompassListener.onCarDataAvailable(compass(90f, CarValue.STATUS_SUCCESS));
    mCarCompassListener.onCarDataAvailable(compass(90f, CarValue.STATUS_SUCCESS));
    mMap.verify(() -> Map.onCompassUpdated(Math.toRadians(110), true), times(2));
    assertEquals(Math.toRadians(140), mSensorHelper.getSavedNorth(), 1e-6);

    mDeclinationDegrees = -10;
    mSensorHelper.updateLocation(mock(Location.class));
    mCarCompassListener.onCarDataAvailable(compass(0f, CarValue.STATUS_SUCCESS));
    mMap.verify(() -> Map.onCompassUpdated(Math.toRadians(350), true));
    assertEquals(Math.toRadians(110), mSensorHelper.getSavedNorth(), 1e-6);
    mMap.verifyNoMoreInteractions();
  }

  @Test
  public void unsupportedCarCompassPassesTheCorrectedPhoneHeadingThroughUnchanged()
  {
    mCarCompassListener.onCarDataAvailable(compass(null, CarValue.STATUS_UNIMPLEMENTED));
    mMap.verify(() -> Map.onCompassUpdated(mSensorHelper.getSavedNorth(), true));
    assertEquals(Math.toRadians(140), mSensorHelper.getSavedNorth(), 1e-6);
    sample(60);
    mMap.verify(() -> Map.onCompassUpdated(mSensorHelper.getSavedNorth(), true));
    assertEquals(Math.toRadians(170), mSensorHelper.getSavedNorth(), 1e-6);
    mMap.verifyNoMoreInteractions();
    mManager.onStop();
    sample(90);
    mMap.verifyNoMoreInteractions();
  }

  @Test
  public void rejectedCarSubscriptionAlsoUsesTheCorrectedPhoneHeading()
  {
    mManager.onStop();
    doThrow(new SecurityException()).when(mCarSensors).addCompassListener(anyInt(), any(), any());
    mManager.onStart();
    mMap.verify(() -> Map.onCompassUpdated(mSensorHelper.getSavedNorth(), true));
    assertEquals(Math.toRadians(140), mSensorHelper.getSavedNorth(), 1e-6);
    mMap.verifyNoMoreInteractions();
  }

  @Test
  public void missingFailedNonFiniteAndInactiveCarSamplesDoNotReachTheMap()
  {
    mCarCompassListener.onCarDataAvailable(compass(null, CarValue.STATUS_SUCCESS));
    mCarCompassListener.onCarDataAvailable(compass(90f, CarValue.STATUS_UNAVAILABLE));
    for (float azimuth : new float[] {Float.NaN, Float.POSITIVE_INFINITY, Float.NEGATIVE_INFINITY})
      mCarCompassListener.onCarDataAvailable(compass(azimuth, CarValue.STATUS_SUCCESS));
    mManager.onStop();
    mCarCompassListener.onCarDataAvailable(compass(90f, CarValue.STATUS_SUCCESS));
    mMap.verifyNoInteractions();
  }

  private void sample(double degrees)
  {
    mYawRadians = (float) Math.toRadians(degrees);
    final SensorEvent event = mock(SensorEvent.class);
    event.accuracy = SensorManager.SENSOR_STATUS_ACCURACY_HIGH;
    mSensorHelper.onSensorChanged(event);
  }

  private static Compass compass(Float azimuth, int status)
  {
    final Compass compass = mock(Compass.class);
    final List<Float> orientations = azimuth == null ? null : List.of(azimuth, 0f, 0f);
    when(compass.getOrientations()).thenReturn(new CarValue<>(orientations, 0L, status));
    return compass;
  }
}
