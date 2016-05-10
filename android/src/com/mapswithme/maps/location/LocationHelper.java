package com.mapswithme.maps.location;

import android.app.Activity;
import android.content.Context;
import android.content.IntentSender;
import android.content.SharedPreferences;
import android.hardware.GeomagneticField;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.location.Location;
import android.preference.PreferenceManager;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import java.util.List;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.google.android.gms.common.api.Status;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.LocationState;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Listeners;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.SimpleLogger;

public enum LocationHelper implements SensorEventListener
{
  INSTANCE;

  protected final Logger mLogger;
  // These constants should correspond to values defined in platform/location.hpp
  // Leave 0-value as no any error.
  public static final int ERROR_NOT_SUPPORTED = 1;
  public static final int ERROR_DENIED = 2;
  public static final int ERROR_GPS_OFF = 3;
  public static final int ERROR_UNKNOWN = 0;

  private static final long INTERVAL_FOLLOW_AND_ROTATE_MS = 3000;
  private static final long INTERVAL_FOLLOW_MS = 3000;
  private static final long INTERVAL_NOT_FOLLOW_MS = 10000;
  private static final long INTERVAL_NAVIGATION_VEHICLE_MS = 500;
  private static final long INTERVAL_NAVIGATION_PEDESTRIAN_MS = 3000;

  public static final String LOCATION_PREDICTOR_PROVIDER = "LocationPredictorProvider";
  private static final float DISTANCE_TO_RECREATE_MAGNETIC_FIELD_M = 1000;
  private static final long STOP_DELAY_MS = 5000;

  private static final String PREF_RESOLVE_ERRORS = "ResolveLocationErrors";

  public interface LocationListener
  {
    void onLocationUpdated(final Location l);
    void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy);
    void onLocationError(int errorCode);
  }

  public static class SimpleLocationListener implements LocationListener
  {
    @Override
    public void onLocationUpdated(Location l) {}

    @Override
    public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy) {}

    @Override
    public void onLocationError(int errorCode) {}
  }

  private final Listeners<LocationListener> mListeners = new Listeners<>();

  private boolean mActive;

  private Location mSavedLocation;
  private MapObject mMyPosition;
  private long mLastLocationTime;

  private final SensorManager mSensorManager;
  private Sensor mAccelerometer;
  private Sensor mMagnetometer;
  private GeomagneticField mMagneticField;
  private BaseLocationProvider mLocationProvider;

  private long mInterval;
  private boolean mHighAccuracy;

  private float[] mGravity;
  private float[] mGeomagnetic;
  private final float[] mR = new float[9];
  private final float[] mI = new float[9];
  private final float[] mOrientation = new float[3];

  private final Runnable mStopLocationTask = new Runnable() {
    @Override
    public void run()
    {
      if (!mActive)
        return;

      mActive = false;
      mLocationProvider.stopUpdates();
      mMagneticField = null;
      if (mSensorManager != null)
        mSensorManager.unregisterListener(LocationHelper.this);
    }
  };

  LocationHelper()
  {
    mLogger = SimpleLogger.get(LocationHelper.class.getName());
    initLocationProvider(false);
    mSensorManager = (SensorManager) MwmApplication.get().getSystemService(Context.SENSOR_SERVICE);
    if (mSensorManager != null)
    {
      mAccelerometer = mSensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
      mMagnetometer = mSensorManager.getDefaultSensor(Sensor.TYPE_MAGNETIC_FIELD);
    }
  }

  public void initLocationProvider(boolean forceNativeProvider)
  {
    final MwmApplication application = MwmApplication.get();
    final boolean containsGoogleServices = GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(application) == ConnectionResult.SUCCESS;
    final boolean googleServicesTurnedInSettings = prefs().getBoolean(application.getString(R.string.pref_play_services), false);
    if (!forceNativeProvider &&
        containsGoogleServices &&
        googleServicesTurnedInSettings)
    {
      mLogger.d("Use fused provider.");
      mLocationProvider = new GoogleFusedLocationProvider();
    }
    else
    {
      mLogger.d("Use native provider.");
      mLocationProvider = new AndroidNativeProvider();
    }

    mActive = !mListeners.isEmpty();
    if (mActive)
      start();
  }

  @Nullable
  public MapObject getMyPosition()
  {
    if (!LocationState.INSTANCE.isTurnedOn())
    {
      mMyPosition = null;
      return null;
    }

    if (mSavedLocation == null)
      return null;

    if (mMyPosition == null)
      mMyPosition = new MapObject(MapObject.MY_POSITION, "", "", "", mSavedLocation.getLatitude(), mSavedLocation.getLongitude(), "");

    return mMyPosition;
  }

  /**
   * <p>Obtains last known saved location. It depends on "My position" button state and is cleared on "No position" one.
   * <p>If you need the location regardless of the button's state, use {@link #getLastKnownLocation()}.
   * @return {@code null} if no location is saved or "My position" button is in "No position" state.
   */
  public Location getSavedLocation() { return mSavedLocation; }

  public long getSavedLocationTime() { return mLastLocationTime; }

  public void saveLocation(@NonNull Location loc)
  {
    mSavedLocation = loc;
    mMyPosition = null;
    mLastLocationTime = System.currentTimeMillis();
    notifyLocationUpdated();
  }

  protected void notifyLocationUpdated()
  {
    if (mSavedLocation == null)
      return;

    for (LocationListener listener : mListeners)
      listener.onLocationUpdated(mSavedLocation);
    mListeners.finishIterate();
  }

  protected void notifyLocationError(int errCode)
  {
    for (LocationListener listener : mListeners)
      listener.onLocationError(errCode);
    mListeners.finishIterate();
  }

  public boolean shouldResolveErrors()
  {
    return prefs().getBoolean(PREF_RESOLVE_ERRORS, true);
  }

  public void setShouldResolveErrors(boolean shouldResolve)
  {
    prefs().edit().putBoolean(PREF_RESOLVE_ERRORS, shouldResolve).apply();
  }

  protected void resolveLocationError(Status status)
  {
    if (!shouldResolveErrors())
      return;

    for (LocationListener listener : mListeners)
    {
      if (listener instanceof Activity)
      {
        // Show the dialog and check the result in onActivityResult().
        try
        {
          status.startResolutionForResult((Activity) listener, MwmActivity.REQUEST_CHECK_SETTINGS);
        } catch (IntentSender.SendIntentException ignored) {}
      }
    }
    mListeners.finishIterate();
  }

  private void notifyCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    for (LocationListener listener : mListeners)
      listener.onCompassUpdated(time, magneticNorth, trueNorth, accuracy);
    mListeners.finishIterate();
  }

  @android.support.annotation.UiThread
  public void addLocationListener(LocationListener listener, boolean forceUpdate)
  {
    UiThread.cancelDelayedTasks(mStopLocationTask);

    if (mListeners.isEmpty())
      start();

    mListeners.register(listener);

    if (forceUpdate)
      notifyLocationUpdated();
  }

  @android.support.annotation.UiThread
  public void removeLocationListener(LocationListener listener)
  {
    boolean wasEmpty = mListeners.isEmpty();
    mListeners.unregister(listener);

    if (!wasEmpty && mListeners.isEmpty())
      // Make a delay with disconnection from location providers, so that orientation changes and short app sleeps
      // doesn't take long time to connect again.
      UiThread.runLater(mStopLocationTask, STOP_DELAY_MS);
  }

  void registerSensorListeners()
  {
    if (mSensorManager != null)
    {
      if (mAccelerometer != null)
        mSensorManager.registerListener(this, mAccelerometer, SensorManager.SENSOR_DELAY_UI);
      if (mMagnetometer != null)
        mSensorManager.registerListener(this, mMagnetometer, SensorManager.SENSOR_DELAY_UI);
    }
  }

  @Override
  public void onSensorChanged(SensorEvent event)
  {
    if (!MwmApplication.get().isFrameworkInitialized())
      return;

    boolean hasOrientation = false;

    switch (event.sensor.getType())
    {
    case Sensor.TYPE_ACCELEROMETER:
      mGravity = nativeUpdateCompassSensor(0, event.values);
      break;
    case Sensor.TYPE_MAGNETIC_FIELD:
      mGeomagnetic = nativeUpdateCompassSensor(1, event.values);
      break;
    }

    if (mGravity != null && mGeomagnetic != null)
    {
      if (SensorManager.getRotationMatrix(mR, mI, mGravity, mGeomagnetic))
      {
        hasOrientation = true;
        SensorManager.getOrientation(mR, mOrientation);
      }
    }

    if (hasOrientation)
    {
      final double magneticHeading = LocationUtils.correctAngle(mOrientation[0], 0.0);

      if (mMagneticField == null)
        notifyCompassUpdated(event.timestamp, magneticHeading, -1.0, -1.0); // -1.0 - as default parameters
      else
      {
        // positive 'offset' means the magnetic field is rotated east that match from true north
        final double offset = Math.toRadians(mMagneticField.getDeclination());
        final double trueHeading = LocationUtils.correctAngle(magneticHeading, offset);

        notifyCompassUpdated(event.timestamp, magneticHeading, trueHeading, offset);
      }
    }
  }

  @Override
  public void onAccuracyChanged(Sensor sensor, int accuracy) {}

  public void initMagneticField(Location newLocation)
  {
    if (mSensorManager != null)
    {
      // Recreate magneticField if location has changed significantly
      if (mMagneticField == null || mSavedLocation == null ||
          newLocation.distanceTo(mSavedLocation) > DISTANCE_TO_RECREATE_MAGNETIC_FIELD_M)
        mMagneticField = new GeomagneticField((float) newLocation.getLatitude(), (float) newLocation.getLongitude(),
                                              (float) newLocation.getAltitude(), newLocation.getTime());
    }
  }

  private void calcParams()
  {
    mHighAccuracy = true;
    if (RoutingController.get().isNavigating())
    {
      mInterval = (Framework.nativeGetRouter() == Framework.ROUTER_TYPE_VEHICLE ? INTERVAL_NAVIGATION_VEHICLE_MS
                                                                                : INTERVAL_NAVIGATION_PEDESTRIAN_MS);
      return;
    }

    int mode = LocationState.INSTANCE.nativeGetMode();
    switch (mode)
    {
    default:
    case LocationState.NOT_FOLLOW:
      mHighAccuracy = false;
      mInterval = INTERVAL_NOT_FOLLOW_MS;
      break;

    case LocationState.FOLLOW:
      mInterval = INTERVAL_FOLLOW_MS;
      break;

    case LocationState.FOLLOW_AND_ROTATE:
      mInterval = INTERVAL_FOLLOW_AND_ROTATE_MS;
      break;
    }
  }

  long getInterval()
  {
    return mInterval;
  }

  boolean isHighAccuracy()
  {
    return mHighAccuracy;
  }

  public void restart()
  {
    mLogger.d("restart");
    mLocationProvider.stopUpdates();

    mActive &= !mListeners.isEmpty();
    if (mActive)
      start();
  }

  private void start()
  {
    mActive = true;
    calcParams();

    mLogger.d("start. Params: " + mInterval + ", high: " + mHighAccuracy);
    mLocationProvider.startUpdates();
  }

  public static void onLocationUpdated(@NonNull Location location)
  {
    nativeLocationUpdated(location.getTime(),
                          location.getLatitude(),
                          location.getLongitude(),
                          location.getAccuracy(),
                          location.getAltitude(),
                          location.getSpeed(),
                          location.getBearing());
  }

  /**
   * Obtains last known location regardless of "My position" button state.
   * @return {@code null} on failure.
   */
  public @Nullable Location getLastKnownLocation(long expirationMs)
  {
    if (mSavedLocation != null)
      return mSavedLocation;

    AndroidNativeProvider provider = new AndroidNativeProvider();
    List<String> providers = provider.getFilteredProviders();

    if (providers.isEmpty())
      return null;

    return provider.findBestNotExpiredLocation(providers, expirationMs);
  }

  public @Nullable Location getLastKnownLocation()
  {
    return getLastKnownLocation(LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_LONG);
  }

  private SharedPreferences prefs()
  {
    return PreferenceManager.getDefaultSharedPreferences(MwmApplication.get());
  }

  public static native void nativeOnLocationError(int errorCode);
  private static native void nativeLocationUpdated(long time, double lat, double lon, float accuracy, double altitude, float speed, float bearing);
  private static native float[] nativeUpdateCompassSensor(int ind, float[] arr);
}
