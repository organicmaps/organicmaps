package app.organicmaps.sdk.location;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockConstruction;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.hardware.GeomagneticField;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorManager;
import android.location.Location;
import android.view.Surface;
import app.organicmaps.sdk.util.log.Logger;
import java.util.ArrayList;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.MockedConstruction;
import org.mockito.MockedStatic;

public class SensorHelperTest
{
  private static final double EPSILON = 1e-6;

  private MockedStatic<Logger> mLogger;
  private MockedStatic<SensorManager> mSensorApi;
  private MockedConstruction<GeomagneticField> mFields;
  private final List<List<?>> mFieldArguments = new ArrayList<>();
  private final SensorManager mManager = mock(SensorManager.class);
  private final SensorListener mListener = mock(SensorListener.class);
  private SensorHelper mHelper;
  private float mYawRadians;
  private float mDeclinationDegrees;

  @Before
  public void setUp()
  {
    mLogger = mockStatic(Logger.class);
    mSensorApi = mockStatic(SensorManager.class);
    mSensorApi.when(() -> SensorManager.getOrientation(any(float[].class), any(float[].class))).thenAnswer(call -> {
      final float[] orientation = call.getArgument(1);
      orientation[0] = mYawRadians;
      return orientation;
    });
    mFields = mockConstruction(GeomagneticField.class, (field, context) -> {
      mFieldArguments.add(List.copyOf(context.arguments()));
      when(field.getDeclination()).thenReturn(mDeclinationDegrees);
    });
    final Context context = mock(Context.class);
    when(context.getSystemService(Context.SENSOR_SERVICE)).thenReturn(mManager);
    mHelper = new SensorHelper(context);
    mHelper.addListener(mListener);
  }

  @After
  public void tearDown()
  {
    mFields.close();
    mSensorApi.close();
    mLogger.close();
  }

  @Test
  public void headingsUseEastWestAndZeroDeclinationForEveryDisplayRotation()
  {
    // Magnetic yaw, declination, then expected headings for rotations 0, 90, 180, 270.
    final double[][] cases = {{0, 20, 20, 110, 200, 290},   {0, -20, 340, 70, 160, 250},   {0, 0, 0, 90, 180, 270},
                              {-30, 20, 350, 80, 170, 260}, {-30, -20, 310, 40, 130, 220}, {-30, 0, 330, 60, 150, 240},
                              {170, 20, 190, 280, 10, 100}, {-170, -20, 170, 260, 350, 80}};
    final int[] rotations = {Surface.ROTATION_0, Surface.ROTATION_90, Surface.ROTATION_180, Surface.ROTATION_270};
    for (double[] test : cases)
    {
      mDeclinationDegrees = (float) test[1];
      mHelper.updateLocation(mock(Location.class));
      sample(test[0], SensorManager.SENSOR_STATUS_ACCURACY_HIGH);
      for (int i = 0; i < rotations.length; i++)
      {
        mHelper.setRotation(rotations[i]);
        assertHeading(test[i + 2], mHelper.getSavedNorth());
      }
    }
  }

  @Test
  public void firstFixCorrectsAndReplaysASavedMagneticSampleImmediately()
  {
    assertTrue(Double.isNaN(mHelper.getSavedNorth()));
    verifyNoInteractions(mListener);
    sample(-10, SensorManager.SENSOR_STATUS_ACCURACY_HIGH);
    assertHeading(350, mHelper.getSavedNorth());
    verify(mListener).onCompassUpdated(mHelper.getSavedNorth());
    final SensorListener replay = mock(SensorListener.class);
    mHelper.addListener(replay);
    verify(replay).onCompassUpdated(mHelper.getSavedNorth());

    clearInvocations(mListener, replay);
    mDeclinationDegrees = 20;
    mHelper.updateLocation(mock(Location.class));
    assertHeading(10, mHelper.getSavedNorth());
    verify(mListener).onCompassUpdated(mHelper.getSavedNorth());
    verify(replay).onCompassUpdated(mHelper.getSavedNorth());
    final SensorListener correctedReplay = mock(SensorListener.class);
    mHelper.addListener(correctedReplay);
    verify(correctedReplay).onCompassUpdated(mHelper.getSavedNorth());
  }

  @Test
  public void locationAndRotationBeforeASampleDoNotPublishAHeading()
  {
    mDeclinationDegrees = -20;
    mHelper.updateLocation(mock(Location.class));
    mHelper.setRotation(Surface.ROTATION_270);
    final SensorListener replay = mock(SensorListener.class);
    mHelper.addListener(replay);
    assertTrue(Double.isNaN(mHelper.getSavedNorth()));
    verifyNoInteractions(mListener, replay);

    sample(20, SensorManager.SENSOR_STATUS_ACCURACY_HIGH);
    assertHeading(270, mHelper.getSavedNorth());
    verify(mListener).onCompassUpdated(mHelper.getSavedNorth());
    verify(replay).onCompassUpdated(mHelper.getSavedNorth());
  }

  @Test
  public void changedAndRepeatedLocationsNeverAccumulateDeclination()
  {
    sample(0, SensorManager.SENSOR_STATUS_ACCURACY_HIGH);
    clearInvocations(mListener);
    final Location location = mock(Location.class);
    mDeclinationDegrees = 20;
    mHelper.updateLocation(location);
    mHelper.updateLocation(location);
    assertHeading(20, mHelper.getSavedNorth());
    verify(mListener, times(2)).onCompassUpdated(mHelper.getSavedNorth());

    mDeclinationDegrees = -10;
    mHelper.updateLocation(mock(Location.class));
    assertHeading(350, mHelper.getSavedNorth());
    verify(mListener).onCompassUpdated(mHelper.getSavedNorth());
    sample(45, SensorManager.SENSOR_STATUS_ACCURACY_HIGH);
    assertHeading(35, mHelper.getSavedNorth());
    assertEquals(3, mFields.constructed().size());
  }

  @Test
  public void rotationRepublishesTheRawSampleWithoutAnotherSensorEvent()
  {
    mDeclinationDegrees = 20;
    mHelper.updateLocation(mock(Location.class));
    sample(-30, SensorManager.SENSOR_STATUS_ACCURACY_HIGH);
    clearInvocations(mListener);

    mHelper.setRotation(Surface.ROTATION_90);
    assertHeading(80, mHelper.getSavedNorth());
    verify(mListener).onCompassUpdated(mHelper.getSavedNorth());
    final SensorListener replay = mock(SensorListener.class);
    mHelper.addListener(replay);
    verify(replay).onCompassUpdated(mHelper.getSavedNorth());
    mHelper.setRotation(Surface.ROTATION_0);
    assertHeading(350, mHelper.getSavedNorth());
    verify(mListener).onCompassUpdated(mHelper.getSavedNorth());

    clearInvocations(mListener);
    mHelper.setRotation(Surface.ROTATION_0);
    verifyNoInteractions(mListener);
    mSensorApi.verify(() -> SensorManager.getOrientation(any(float[].class), any(float[].class)));
    assertEquals(1, mFields.constructed().size());
  }

  @Test
  public void modelUsesLatitudeLongitudeWgs84AltitudeAndCurrentObservationTime()
  {
    final Location location = mock(Location.class);
    when(location.getLatitude()).thenReturn(47.123456);
    when(location.getLongitude()).thenReturn(-122.654321);
    when(location.hasAltitude()).thenReturn(true);
    when(location.getAltitude()).thenReturn(1234.5);
    when(location.getTime()).thenReturn(1L);
    final long before = System.currentTimeMillis();
    mHelper.updateLocation(location);
    final long after = System.currentTimeMillis();
    final List<?> arguments = mFieldArguments.get(0);
    assertEquals((float) 47.123456, arguments.get(0));
    assertEquals((float) -122.654321, arguments.get(1));
    assertEquals(1234.5f, arguments.get(2));
    assertTrue((long) arguments.get(3) >= before);
    assertTrue((long) arguments.get(3) <= after);

    when(location.hasAltitude()).thenReturn(false);
    mHelper.updateLocation(location);
    assertEquals(0f, mFieldArguments.get(1).get(2));
  }

  @Test
  public void unreliableSamplesPreserveTheLastHeadingAndCalibrationNotifications()
  {
    sample(30, SensorManager.SENSOR_STATUS_ACCURACY_HIGH);
    clearInvocations(mListener);
    sample(90, SensorManager.SENSOR_STATUS_UNRELIABLE);
    sample(90, SensorManager.SENSOR_STATUS_UNRELIABLE);
    assertHeading(30, mHelper.getSavedNorth());
    verify(mListener).onCompassCalibrationRequired();
    mSensorApi.verify(() -> SensorManager.getOrientation(any(float[].class), any(float[].class)));

    mDeclinationDegrees = 20;
    mHelper.updateLocation(mock(Location.class));
    assertHeading(50, mHelper.getSavedNorth());
    sample(45, SensorManager.SENSOR_STATUS_ACCURACY_MEDIUM);
    sample(60, SensorManager.SENSOR_STATUS_ACCURACY_MEDIUM);
    assertHeading(80, mHelper.getSavedNorth());
    verify(mListener).onCompassCalibrationRecommended();
    sample(90, SensorManager.SENSOR_STATUS_ACCURACY_LOW);
    assertHeading(110, mHelper.getSavedNorth());
    verify(mListener, times(2)).onCompassCalibrationRequired();
  }

  @Test
  public void correctionAndSampleSurviveStopStart()
  {
    final Sensor sensor = mock(Sensor.class);
    when(mManager.getDefaultSensor(Sensor.TYPE_ROTATION_VECTOR)).thenReturn(sensor);
    mHelper.start();
    mDeclinationDegrees = -20;
    mHelper.updateLocation(mock(Location.class));
    sample(30, SensorManager.SENSOR_STATUS_ACCURACY_HIGH);
    mHelper.stop();
    mHelper.start();
    assertHeading(10, mHelper.getSavedNorth());
    final SensorListener replay = mock(SensorListener.class);
    mHelper.addListener(replay);
    verify(replay).onCompassUpdated(mHelper.getSavedNorth());
    assertEquals(1, mFields.constructed().size());
  }

  private void sample(double degrees, int accuracy)
  {
    mYawRadians = (float) Math.toRadians(degrees);
    final SensorEvent event = mock(SensorEvent.class);
    event.accuracy = accuracy;
    mHelper.onSensorChanged(event);
  }

  private static void assertHeading(double degrees, double actual)
  {
    assertEquals(Math.toRadians(degrees), actual, EPSILON);
  }
}
