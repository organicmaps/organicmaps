package com.mapswithme.maps.location;

import android.app.Activity;
import android.location.Location;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.UiThread;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Config;
import com.mapswithme.util.Listeners;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;

public enum LocationHelper
{
  INSTANCE;

  // These constants should correspond to values defined in platform/location.hpp
  // Leave 0-value as no any error.
  public static final int ERROR_NOT_SUPPORTED = 1;
  public static final int ERROR_DENIED = 2;
  public static final int ERROR_GPS_OFF = 3;
  public static final int ERROR_UNKNOWN = 0;

  private static final long INTERVAL_FOLLOW_AND_ROTATE_MS = 3000;
  private static final long INTERVAL_FOLLOW_MS = 1000;
  private static final long INTERVAL_NOT_FOLLOW_MS = 3000;
  private static final long INTERVAL_NAVIGATION_VEHICLE_MS = 500;

  // TODO (trashkalmar): Correct value
  private static final long INTERVAL_NAVIGATION_BICYCLE_MS = 1000;
  private static final long INTERVAL_NAVIGATION_PEDESTRIAN_MS = 1000;

  @NonNull
  private final TransitionListener mOnTransition = new TransitionListener();

  @NonNull
  private final LocationListener mCoreLocationListener = new LocationListener()
  {
    @Override
    public void onLocationUpdated(Location location)
    {
      // If we are still in the first run mode, i.e. user is staying on the first run screens,
      // not on the map, we mustn't post location update to the core. Only this preserving allows us
      // to play nice zoom animation once a user will leave first screens and will see a map.
      if (mInFirstRun)
      {
        mLogger.d(TAG, "Location update is obtained and must be ignored, " +
                       "because the app is in a first run mode");
        return;
      }

      nativeLocationUpdated(location.getTime(),
                            location.getLatitude(),
                            location.getLongitude(),
                            location.getAccuracy(),
                            location.getAltitude(),
                            location.getSpeed(),
                            location.getBearing());

      if (mUiCallback != null)
        mUiCallback.onLocationUpdated(location);
    }

    @Override
    public void onCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
    {
      if (mCompassData == null)
        mCompassData = new CompassData();

      mCompassData.update(magneticNorth, trueNorth);

      if (mUiCallback != null)
        mUiCallback.onCompassUpdated(mCompassData);
    }


    @Override
    public void onLocationError(int errorCode)
    {
      mLogger.d(TAG, "onLocationError errorCode = " + errorCode, new Throwable());

      nativeOnLocationError(errorCode);
      mLogger.d(TAG, "nativeOnLocationError errorCode = " + errorCode +
                ", current state = " + LocationState.nameOf(getMyPositionMode()));

      if (mUiCallback == null)
        return;

      mUiCallback.onLocationError();
    }

    @Override
    public String toString()
    {
      return "LocationHelper.mCoreLocationListener";
    }
  };

  private final static String TAG = LocationHelper.class.getSimpleName();
  private final Logger mLogger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.LOCATION);
  @NonNull
  private final Listeners<LocationListener> mListeners = new Listeners<>();
  private Location mSavedLocation;
  private MapObject mMyPosition;
  private long mSavedLocationTime;
  @NonNull
  private final SensorHelper mSensorHelper = new SensorHelper();
  @Nullable
  private BaseLocationProvider mLocationProvider;
  @Nullable
  private UiCallback mUiCallback;
  private long mInterval;
  private CompassData mCompassData;
  private boolean mInFirstRun;
  private boolean mLocationUpdateStoppedByUser;

  @SuppressWarnings("FieldCanBeLocal")
  private final LocationState.ModeChangeListener mMyPositionModeListener =
      new LocationState.ModeChangeListener()
  {
    @Override
    public void onMyPositionModeChanged(int newMode)
    {
      notifyMyPositionModeChanged(newMode);
      mLogger.d(TAG, "onMyPositionModeChanged mode = " + LocationState.nameOf(newMode));

      if (mUiCallback == null)
      {
        mLogger.d(TAG, "UI is not ready to listen my position changes, i.e. it's not attached yet.");
        return;
      }

      switch (newMode)
      {
        case LocationState.NOT_FOLLOW_NO_POSITION:
          // In the first run mode, the NOT_FOLLOW_NO_POSITION state doesn't mean that location
          // is actually not found.
          if (mInFirstRun)
          {
            mLogger.i(TAG, "It's the first run, so this state should be skipped");
            return;
          }

          stop();
          if (LocationUtils.areLocationServicesTurnedOn())
            notifyLocationNotFound();
          break;
      }
    }
  };

  @UiThread
  public void initialize()
  {
    initProvider();
    LocationState.nativeSetListener(mMyPositionModeListener);
    MwmApplication.backgroundTracker().addListener(mOnTransition);
  }

  private void initProvider()
  {
    mLogger.d(TAG, "initProvider", new Throwable());
    final MwmApplication application = MwmApplication.get();
    final boolean containsGoogleServices = GoogleApiAvailability.getInstance().isGooglePlayServicesAvailable(application) == ConnectionResult.SUCCESS;
    final boolean googleServicesTurnedInSettings = Config.useGoogleServices();
    if (containsGoogleServices && googleServicesTurnedInSettings)
    {
      mLogger.d(TAG, "Use fused provider.");
      mLocationProvider = new GoogleFusedLocationProvider(new FusedLocationFixChecker());
    }
    else
    {
      initNativeProvider();
    }
  }

  void initNativeProvider()
  {
    mLogger.d(TAG, "Use native provider");
    mLocationProvider = new AndroidNativeProvider(new DefaultLocationFixChecker());
  }

  public void onLocationUpdated(@NonNull Location location)
  {
    mSavedLocation = location;
    mMyPosition = null;
    mSavedLocationTime = System.currentTimeMillis();
  }

  /**
   * @return MapObject.MY_POSITION, null if location is not yet determined or "My position" button is switched off.
   */
  @Nullable
  public MapObject getMyPosition()
  {
    if (!LocationState.isTurnedOn())
    {
      mMyPosition = null;
      return null;
    }

    if (mSavedLocation == null)
      return null;

    if (mMyPosition == null)
      mMyPosition = MapObject.createMapObject(FeatureId.EMPTY, MapObject.MY_POSITION, "", "",
                                  mSavedLocation.getLatitude(), mSavedLocation.getLongitude());

    return mMyPosition;
  }

  /**
   * <p>Obtains last known saved location. It depends on "My position" button mode and is erased on "No follow, no position" one.
   * <p>If you need the location regardless of the button's state, use {@link #getLastKnownLocation()}.
   * @return {@code null} if no location is saved or "My position" button is in "No follow, no position" mode.
   */
  @Nullable
  public Location getSavedLocation() { return mSavedLocation; }

  public long getSavedLocationTime() { return mSavedLocationTime; }

  public void switchToNextMode()
  {
    mLogger.d(TAG, "switchToNextMode()");
    LocationState.nativeSwitchToNextMode();
  }

  /**
   * Indicates about whether a location provider is polling location updates right now or not.
   * @see BaseLocationProvider#isActive()
   */
  public boolean isActive()
  {
    return mLocationProvider != null && mLocationProvider.isActive();
  }

  public void setStopLocationUpdateByUser(boolean isStopped)
  {
    mLogger.d(TAG, "Set stop location update by user: " + isStopped);
    mLocationUpdateStoppedByUser = isStopped;
  }

  private boolean isLocationUpdateStoppedByUser()
  {
    return mLocationUpdateStoppedByUser;
  }

  void notifyCompassUpdated(long time, double magneticNorth, double trueNorth, double accuracy)
  {
    for (LocationListener listener : mListeners)
      listener.onCompassUpdated(time, magneticNorth, trueNorth, accuracy);
    mListeners.finishIterate();
  }

  void notifyLocationUpdated()
  {
    if (mSavedLocation == null)
    {
      mLogger.d(TAG, "No saved location - skip");
      return;
    }

    for (LocationListener listener : mListeners)
      listener.onLocationUpdated(mSavedLocation);
    mListeners.finishIterate();

    // TODO: consider to create callback mechanism to transfer 'ROUTE_IS_FINISHED' event from
    // the core to the platform code (https://jira.mail.ru/browse/MAPSME-3675),
    // because calling the native method 'nativeIsRouteFinished'
    // too often can result in poor UI performance.
    if (RoutingController.get().isNavigating() && Framework.nativeIsRouteFinished())
    {
      mLogger.d(TAG, "End point is reached");
      restart();
      RoutingController.get().cancel();
    }
  }

  private void notifyLocationUpdated(LocationListener listener)
  {
    mLogger.d(TAG, "notifyLocationUpdated(), listener: " + listener);

    if (mSavedLocation == null)
    {
      mLogger.d(TAG, "No saved location - skip");
      return;
    }

    listener.onLocationUpdated(mSavedLocation);
  }

  private void notifyLocationError(int errCode)
  {
    mLogger.d(TAG, "notifyLocationError(): " + errCode);

    for (LocationListener listener : mListeners)
      listener.onLocationError(errCode);
    mListeners.finishIterate();
  }

  private void notifyMyPositionModeChanged(int newMode)
  {
    mLogger.d(TAG, "notifyMyPositionModeChanged(): " + LocationState.nameOf(newMode) , new Throwable());

    if (mUiCallback != null)
      mUiCallback.onMyPositionModeChanged(newMode);
  }

  private void notifyLocationNotFound()
  {
    mLogger.d(TAG, "notifyLocationNotFound()");
    if (mUiCallback != null)
      mUiCallback.onLocationNotFound();
  }

  /**
   * Registers listener about location changes.
   *
   * @param listener    listener to register.
   * @param forceUpdate instantly notify given listener about available location, if any.
   */
  @UiThread
  public void addListener(@NonNull LocationListener listener, boolean forceUpdate)
  {
    mLogger.d(TAG, "addListener(): " + listener + ", forceUpdate: " + forceUpdate);
    mLogger.d(TAG, " - listener count was: " + mListeners.getSize());

    mListeners.register(listener);

    if (forceUpdate)
      notifyLocationUpdated(listener);
  }

  @UiThread
  /**
   * Removes given location listener.
   * @param listener listener to unregister.
   */
  public void removeListener(@NonNull LocationListener listener)
  {
    mLogger.d(TAG, "removeListener(), listener: " + listener);
    mLogger.d(TAG, " - listener count was: " + mListeners.getSize());
    mListeners.unregister(listener);
  }

  void startSensors()
  {
    mSensorHelper.start();
  }

  void resetMagneticField(Location location)
  {
    mSensorHelper.resetMagneticField(mSavedLocation, location);
  }

  private void calcLocationUpdatesInterval()
  {
    mLogger.d(TAG, "calcLocationUpdatesInterval()");
    if (RoutingController.get().isNavigating())
    {
      mLogger.d(TAG, "calcLocationUpdatesInterval(), it's navigation mode");
      final @Framework.RouterType int router = Framework.nativeGetRouter();
      switch (router)
      {
      case Framework.ROUTER_TYPE_PEDESTRIAN:
        mInterval = INTERVAL_NAVIGATION_PEDESTRIAN_MS;
        break;

      case Framework.ROUTER_TYPE_VEHICLE:
        mInterval = INTERVAL_NAVIGATION_VEHICLE_MS;
        break;

      case Framework.ROUTER_TYPE_BICYCLE:
        mInterval = INTERVAL_NAVIGATION_BICYCLE_MS;
        break;

      case Framework.ROUTER_TYPE_TRANSIT:
        // TODO: what is the interval should be for transit type?
        mInterval = INTERVAL_NAVIGATION_PEDESTRIAN_MS;
        break;

      default:
        throw new IllegalArgumentException("Unsupported router type: " + router);
      }

      return;
    }

    int mode = getMyPositionMode();
    switch (mode)
    {
      case LocationState.FOLLOW:
        mInterval = INTERVAL_FOLLOW_MS;
        break;

      case LocationState.FOLLOW_AND_ROTATE:
        mInterval = INTERVAL_FOLLOW_AND_ROTATE_MS;
        break;

      default:
        mInterval = INTERVAL_NOT_FOLLOW_MS;
        break;
    }
  }

  long getInterval()
  {
    return mInterval;
  }

  /**
   * Stops the current provider. Then initialize the location provider again,
   * because location settings could be changed and a new location provider can be used,
   * such as Google fused provider. And we think that Google fused provider is preferable
   * for the most cases. And starts the initialized location provider.
   *
   * @see #start()
   *
   */
  public void restart()
  {
    mLogger.d(TAG, "restart()");
    checkProviderInitialization();
    stopInternal();
    initProvider();
    start();
  }

  /**
   * Adds the {@link #mCoreLocationListener} to listen location updates and notify UI.
   * Notifies about {@link #ERROR_DENIED} if there are no enabled location providers.
   * Calculates minimum time interval for location updates.
   * Starts polling location updates.
   */
  public void start()
  {
    if (isLocationUpdateStoppedByUser())
    {
      mLogger.d(TAG, "Location updates are stopped by the user manually, so skip provider start"
                     + " until the user starts it manually.");
      return;
    }

    checkProviderInitialization();
    //noinspection ConstantConditions
    if (mLocationProvider.isActive())
      throw new AssertionError("Location provider '" + mLocationProvider
                               + "' must be stopped first");

    addListener(mCoreLocationListener, true);

    if (!LocationUtils.checkProvidersAvailability())
    {
      // No need to notify about an error in first run mode
      if (!mInFirstRun)
        notifyLocationError(ERROR_DENIED);
      return;
    }

    long oldInterval = mInterval;
    mLogger.d(TAG, "Old time interval (ms): " + oldInterval);
    calcLocationUpdatesInterval();
    mLogger.d(TAG, "start(), params: " + mInterval);
    startInternal();
  }

  /**
   * Stops the polling location updates, i.e. removes the {@link #mCoreLocationListener} and stops
   * the current active provider.
   */
  private void stop()
  {
    mLogger.d(TAG, "stop()");
    checkProviderInitialization();
    //noinspection ConstantConditions
    if (!mLocationProvider.isActive())
    {
      mLogger.i(TAG, "Provider '" + mLocationProvider + "' is already stopped");
      return;
    }

    removeListener(mCoreLocationListener);
    stopInternal();
  }

  /**
   * Actually starts location polling.
   */
  private void startInternal()
  {
    mLogger.d(TAG, "startInternal(), current provider is '" + mLocationProvider
                   + "' , my position mode = " + LocationState.nameOf(getMyPositionMode())
                   + ", mInFirstRun = " + mInFirstRun);
    if (!PermissionsUtils.isLocationGranted())
    {
      mLogger.w(TAG, "Dynamic permission ACCESS_COARSE_LOCATION/ACCESS_FINE_LOCATION is not granted",
                new Throwable());
      return;
    }
    checkProviderInitialization();
    //noinspection ConstantConditions
    mLocationProvider.start();
    mLogger.d(TAG, mLocationProvider.isActive() ? "SUCCESS" : "FAILURE");

    if (mLocationProvider.isActive())
    {
      if (!mInFirstRun && getMyPositionMode() == LocationState.NOT_FOLLOW_NO_POSITION)
        switchToNextMode();
    }
  }

  private void checkProviderInitialization()
  {
    if (mLocationProvider == null)
    {
      String error = "A location provider must be initialized!";
      mLogger.e(TAG, error, new Throwable());
      throw new AssertionError(error);
    }
  }

  /**
   * Actually stops location polling.
   */
  private void stopInternal()
  {
    mLogger.d(TAG, "stopInternal()");
    checkProviderInitialization();
    //noinspection ConstantConditions
    mLocationProvider.stop();
    mSensorHelper.stop();
  }

  /**
   * Attach UI to helper.
   */
  @UiThread
  public void attach(@NonNull UiCallback callback)
  {
    mLogger.d(TAG, "attach() callback = " + callback);

    if (mUiCallback != null)
    {
      mLogger.d(TAG, " - already attached. Skip.");
      return;
    }

    mUiCallback = callback;

    Utils.keepScreenOn(true, mUiCallback.getActivity().getWindow());

    mUiCallback.onMyPositionModeChanged(getMyPositionMode());
    if (mCompassData != null)
      mUiCallback.onCompassUpdated(mCompassData);

    checkProviderInitialization();
    //noinspection ConstantConditions
    if (mLocationProvider.isActive())
    {
      mLogger.d(TAG, "attach() provider '" + mLocationProvider + "' is active, just add the listener");
      addListener(mCoreLocationListener, true);
    }
    else
    {
      restart();
    }
  }

  /**
   * Detach UI from helper.
   */
  @UiThread
  public void detach(boolean delayed)
  {
    mLogger.d(TAG, "detach(), delayed: " + delayed);

    if (mUiCallback == null)
    {
      mLogger.d(TAG, " - already detached. Skip.");
      return;
    }

    Utils.keepScreenOn(false, mUiCallback.getActivity().getWindow());
    mUiCallback = null;
    stop();
  }

  @UiThread
  public void onEnteredIntoFirstRun()
  {
    mLogger.i(TAG, "onEnteredIntoFirstRun");
    mInFirstRun = true;
  }

  @UiThread
  public void onExitFromFirstRun()
  {
    mLogger.i(TAG, "onExitFromFirstRun");
    if (!mInFirstRun)
      throw new AssertionError("Must be called only after 'onEnteredIntoFirstRun' method!");

    mInFirstRun = false;

    if (getMyPositionMode() != LocationState.NOT_FOLLOW_NO_POSITION)
      throw new AssertionError("My position mode must be equal NOT_FOLLOW_NO_POSITION");

    // If there is a location we need just to pass it to the listeners, so that
    // my position state machine will be switched to the FOLLOW state.
    Location location = getSavedLocation();
    if (location != null)
    {
      notifyLocationUpdated();
      mLogger.d(TAG, "Current location is available, so play the nice zoom animation");
      Framework.nativeRunFirstLaunchAnimation();
      return;
    }

    checkProviderInitialization();
    // If the location hasn't been obtained yet we need to switch to the next mode and wait for locations.
    // Otherwise, try to restart location updates polling.
    // noinspection ConstantConditions
    if (mLocationProvider.isActive())
      switchToNextMode();
    else
      restart();
  }

  /**
   * Obtains last known location regardless of "My position" button state.
   * @return {@code null} on failure.
   */
  @Nullable
  public Location getLastKnownLocation(long expirationMillis)
  {
    if (mSavedLocation != null)
      return mSavedLocation;

    return AndroidNativeProvider.findBestNotExpiredLocation(expirationMillis);
  }

  @Nullable
  public Location getLastKnownLocation()
  {
    return getLastKnownLocation(LocationUtils.LOCATION_EXPIRATION_TIME_MILLIS_LONG);
  }

  @Nullable
  public CompassData getCompassData()
  {
    return mCompassData;
  }

  @LocationState.Value
  public int getMyPositionMode()
  {
    return LocationState.nativeGetMode();
  }

  private static native void nativeOnLocationError(int errorCode);
  private static native void nativeLocationUpdated(long time, double lat, double lon, float accuracy,
                                                   double altitude, float speed, float bearing);
  static native float[] nativeUpdateCompassSensor(int ind, float[] arr);

  public interface UiCallback
  {
    Activity getActivity();
    void onMyPositionModeChanged(int newMode);
    void onLocationUpdated(@NonNull Location location);
    void onCompassUpdated(@NonNull CompassData compass);
    void onLocationError();
    void onLocationNotFound();
  }
}
