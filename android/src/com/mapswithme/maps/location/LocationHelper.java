package com.mapswithme.maps.location;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.Location;
import android.location.LocationManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.background.AppBackgroundTracker;
import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Config;
import com.mapswithme.util.Listeners;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;

public enum LocationHelper implements Initializable<Context>, AppBackgroundTracker.OnTransitionListener, BaseLocationProvider.Listener
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

  private static final long AGPS_EXPIRATION_TIME_MS = 16 * 60 * 60 * 1000; // 16 hours

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Context mContext;

  private final GPSCheck mReceiver = new GPSCheck();
  private boolean mReceiverRegistered;

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
        Logger.d(TAG, "Location update is obtained and must be ignored, because the app is in a first run mode");
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
    public void onCompassUpdated(long time, double north)
    {
      if (mCompassData == null)
        mCompassData = new CompassData();

      mCompassData.update(mContext, north);

      if (mUiCallback != null)
        mUiCallback.onCompassUpdated(mCompassData);
    }


    @Override
    public void onLocationError(int errorCode)
    {
      Logger.d(TAG, "onLocationError errorCode = " + errorCode +
               ", current state = " + LocationState.nameOf(getMyPositionMode()));
      mSavedLocation = null;
      nativeOnLocationError(errorCode);
      if (mUiCallback != null)
        mUiCallback.onLocationError(errorCode);
    }

    @Override
    public String toString()
    {
      return "LocationHelper.mCoreLocationListener";
    }
  };

  private static final String TAG = LocationHelper.class.getSimpleName();
  @NonNull
  private final Listeners<LocationListener> mListeners = new Listeners<>();
  @Nullable
  private Location mSavedLocation;
  private MapObject mMyPosition;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private SensorHelper mSensorHelper;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private BaseLocationProvider mLocationProvider;
  @Nullable
  private UiCallback mUiCallback;
  private long mInterval;
  private CompassData mCompassData;
  private boolean mInFirstRun;
  private boolean mActive;
  private boolean mLocationUpdateStoppedByUser;

  @SuppressWarnings("FieldCanBeLocal")
  private final LocationState.ModeChangeListener mMyPositionModeListener =
      new LocationState.ModeChangeListener()
  {
    @Override
    public void onMyPositionModeChanged(int newMode)
    {
      notifyMyPositionModeChanged(newMode);
      Logger.d(TAG, "onMyPositionModeChanged mode = " + LocationState.nameOf(newMode));

      if (mUiCallback == null)
        Logger.d(TAG, "UI is not ready to listen my position changes, i.e. it's not attached yet.");
    }
  };

  @SuppressWarnings("FieldCanBeLocal")
  private final LocationState.LocationPendingTimeoutListener mLocationPendingTimeoutListener = () -> {
    if (mActive)
    {
      if (PermissionsUtils.isLocationGranted(mContext) && LocationUtils.areLocationServicesTurnedOn(mContext))
        notifyLocationNotFound();
    }
  };

  @Override
  public void initialize(@NonNull Context context)
  {
    mContext = context;
    mSensorHelper = new SensorHelper(context);
    mLocationProvider = LocationProviderFactory.getProvider(mContext, this);
    LocationState.nativeSetListener(mMyPositionModeListener);
    LocationState.nativeSetLocationPendingTimeoutListener(mLocationPendingTimeoutListener);
    MwmApplication.backgroundTracker(context).addListener(this);
    addListener(mCoreLocationListener);
  }

  @Override
  public void destroy()
  {
    // No op.
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
   * Obtains last known location.
   * @return {@code null} if no location is saved.
   */
  @Nullable
  public Location getSavedLocation() { return mSavedLocation; }

  public void switchToNextMode()
  {
    Logger.d(TAG, "switchToNextMode()");
    LocationState.nativeSwitchToNextMode();
  }

  /**
   * Indicates about whether a location provider is polling location updates right now or not.
   */
  public boolean isActive()
  {
    return mActive;
  }

  public void setStopLocationUpdateByUser(boolean isStopped)
  {
    Logger.d(TAG, "Set stop location update by user: " + isStopped);
    mLocationUpdateStoppedByUser = isStopped;
  }

  private boolean isLocationUpdateStoppedByUser()
  {
    return mLocationUpdateStoppedByUser;
  }

  @Override
  public void onTransit(boolean foreground)
  {
    if (foreground)
    {
      Logger.d(TAG, "Resumed in foreground");

      if (mReceiverRegistered)
      {
        MwmApplication.from(mContext).unregisterReceiver(mReceiver);
        mReceiverRegistered = false;
      }

      initialStart();
    }
    else
    {
      Logger.d(TAG, "Stopped in background");

      if (!mReceiverRegistered)
      {
        final IntentFilter filter = new IntentFilter();
        filter.addAction(LocationManager.PROVIDERS_CHANGED_ACTION);
        filter.addCategory(Intent.CATEGORY_DEFAULT);

        MwmApplication.from(mContext).registerReceiver(mReceiver, filter);
        mReceiverRegistered = true;
      }

      stop();
    }
  }

  void notifyCompassUpdated(long time, double north)
  {
    for (LocationListener listener : mListeners)
      listener.onCompassUpdated(time, north);
    mListeners.finishIterate();
  }

  private void notifyLocationUpdated()
  {
    if (mSavedLocation == null)
      throw new IllegalStateException("No saved location");

    for (LocationListener listener : mListeners)
      listener.onLocationUpdated(mSavedLocation);
    mListeners.finishIterate();

    // TODO: consider to create callback mechanism to transfer 'ROUTE_IS_FINISHED' event from
    // the core to the platform code (https://jira.mail.ru/browse/MAPSME-3675),
    // because calling the native method 'nativeIsRouteFinished'
    // too often can result in poor UI performance.
    if (RoutingController.get().isNavigating() && Framework.nativeIsRouteFinished())
    {
      Logger.d(TAG, "End point is reached");
      restart();
      if (mUiCallback != null)
        mUiCallback.onRoutingFinish();
      RoutingController.get().cancel();
    }
  }

  @Override
  public void onLocationChanged(@NonNull Location location)
  {
    Logger.d(TAG, "onLocationChanged, location = " + location);

    if (!LocationUtils.isAccuracySatisfied(location))
    {
      Logger.w(TAG, "Unsatisfied accuracy for location = " + location);
      return;
    }

    if (mSavedLocation != null)
    {
      final boolean isTrustedFused = mLocationProvider.trustFusedLocations() && LocationUtils.isFromFusedProvider(location);
      if (!isTrustedFused && !LocationUtils.isLocationBetterThanLast(location, mSavedLocation))
      {
        Logger.d(TAG, "The new " + location + " is worse than the last " + mSavedLocation);
        return;
      }
    }

    mSavedLocation = location;
    mMyPosition = null;
    notifyLocationUpdated();
  }

  @Override
  public void onLocationError(int errCode)
  {
    Logger.d(TAG, "onLocationError(): " + errCode);
    if (errCode == ERROR_NOT_SUPPORTED &&
        LocationUtils.areLocationServicesTurnedOn(mContext) &&
        !(mLocationProvider instanceof AndroidNativeProvider))
    {
      // If location service is enabled, try to downgrade to the native provider first
      // and restart the service before notifying the user.
      Logger.d(TAG, "Downgrading to use native provider");
      mLocationProvider = new AndroidNativeProvider(mContext, this);
      restart();
      return;
    }

    for (LocationListener listener : mListeners)
      listener.onLocationError(errCode);
    mListeners.finishIterate();
  }

  private void notifyMyPositionModeChanged(int newMode)
  {
    Logger.d(TAG, "notifyMyPositionModeChanged(): " + LocationState.nameOf(newMode));

    if (mUiCallback != null)
      mUiCallback.onMyPositionModeChanged(newMode);
  }

  private void notifyLocationNotFound()
  {
    Logger.d(TAG, "notifyLocationNotFound()");
    if (mUiCallback != null)
      mUiCallback.onLocationNotFound();
  }

  /**
   * Registers listener to obtain location updates.
   *
   * @param listener    listener to be registered.
   */
  @UiThread
  public void addListener(@NonNull LocationListener listener)
  {
    Logger.d(TAG, "addListener(): " + listener);
    Logger.d(TAG, " - listener count was: " + mListeners.getSize());

    mListeners.register(listener);
    if (mSavedLocation != null)
      listener.onLocationUpdated(mSavedLocation);
  }

  @UiThread
  /**
   * Removes given location listener.
   * @param listener listener to unregister.
   */
  public void removeListener(@NonNull LocationListener listener)
  {
    Logger.d(TAG, "removeListener(), listener: " + listener);
    Logger.d(TAG, " - listener count was: " + mListeners.getSize());
    mListeners.unregister(listener);
  }

  private void calcLocationUpdatesInterval()
  {
    Logger.d(TAG, "calcLocationUpdatesInterval()");
    if (RoutingController.get().isNavigating())
    {
      Logger.d(TAG, "calcLocationUpdatesInterval(), it's navigation mode");
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
    stop();
    start();
  }

  private void initialStart()
  {
    if (LocationState.nativeGetMode() != LocationState.NOT_FOLLOW_NO_POSITION)
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
    if (mActive)
    {
      Logger.w(TAG, "Provider '" + mLocationProvider + "' is already started");
      return;
    }

    if (isLocationUpdateStoppedByUser())
    {
      Logger.d(TAG, "Location updates are stopped by the user manually, so skip provider start"
               + " until the user starts it manually.");
      onLocationError(ERROR_GPS_OFF);
      return;
    }

    long oldInterval = mInterval;
    Logger.d(TAG, "Old time interval (ms): " + oldInterval);
    calcLocationUpdatesInterval();
    if (!PermissionsUtils.isLocationGranted(mContext))
    {
      Logger.w(TAG, "Dynamic permissions ACCESS_COARSE_LOCATION and/or ACCESS_FINE_LOCATION are granted");
      onLocationError(ERROR_DENIED);
      return;
    }
    Logger.i(TAG, "start(): interval = " + mInterval + " provider = '" + mLocationProvider + "' mInFirstRun = " + mInFirstRun);
    checkForAgpsUpdates();
    mLocationProvider.start(mInterval);
    mSensorHelper.start();
    mActive = true;
  }

  /**
   * Stops the polling location updates, i.e. removes the {@link #mCoreLocationListener} and stops
   * the current active provider.
   */
  public void stop()
  {
    Logger.i(TAG, "stop()");
    if (!mActive)
    {
      Logger.w(TAG, "Provider '" + mLocationProvider + "' is already stopped");
      return;
    }

    mLocationProvider.stop();
    mSensorHelper.stop();
    mActive = false;
  }

  private void checkForAgpsUpdates()
  {
    if (!NetworkPolicy.getCurrentNetworkUsageStatus())
      return;

    long previousTimestamp = Config.getAgpsTimestamp();
    long currentTimestamp = System.currentTimeMillis();
    if (previousTimestamp + AGPS_EXPIRATION_TIME_MS > currentTimestamp)
    {
      Logger.d(TAG, "A-GPS should be up to date");
      return;
    }

    Logger.d(TAG, "Requesting new A-GPS data");
    Config.setAgpsTimestamp(currentTimestamp);
    final LocationManager manager = (LocationManager) mContext.getSystemService(Context.LOCATION_SERVICE);
    manager.sendExtraCommand(LocationManager.GPS_PROVIDER, "force_xtra_injection", null);
    manager.sendExtraCommand(LocationManager.GPS_PROVIDER, "force_time_injection", null);
  }

  /**
   * Attach UI to helper.
   */
  @UiThread
  public void attach(@NonNull UiCallback callback)
  {
    Logger.d(TAG, "attach() callback = " + callback);

    if (mUiCallback != null)
    {
      Logger.d(TAG, " - already attached. Skip.");
      return;
    }

    mUiCallback = callback;

    if (!Config.isScreenSleepEnabled()) {
      Utils.keepScreenOn(true, mUiCallback.requireActivity().getWindow());
    }

    mUiCallback.onMyPositionModeChanged(getMyPositionMode());
    if (mCompassData != null)
      mUiCallback.onCompassUpdated(mCompassData);

    if (mActive)
    {
      Logger.d(TAG, "attach() provider '" + mLocationProvider + "' is active, just add the listener");
      if (mSavedLocation != null)
        mCoreLocationListener.onLocationUpdated(mSavedLocation);
    }
    else
    {
      initialStart();
    }
  }

  /**
   * Detach UI from helper.
   */
  @UiThread
  public void detach(boolean delayed)
  {
    Logger.d(TAG, "detach(), delayed: " + delayed);

    if (mUiCallback == null)
    {
      Logger.d(TAG, " - already detached. Skip.");
      return;
    }

    Utils.keepScreenOn(false, mUiCallback.requireActivity().getWindow());
    mUiCallback = null;
  }

  @UiThread
  public boolean isInFirstRun()
  {
    return mInFirstRun;
  }

  @UiThread
  public void onEnteredIntoFirstRun()
  {
    Logger.i(TAG, "onEnteredIntoFirstRun");
    mInFirstRun = true;
  }

  @UiThread
  public void onExitFromFirstRun()
  {
    Logger.i(TAG, "onExitFromFirstRun");
    if (!mInFirstRun)
      throw new AssertionError("Must be called only after 'onEnteredIntoFirstRun' method!");

    mInFirstRun = false;

    // If there is a location we need just to pass it to the listeners, so that
    // my position state machine will be switched to the FOLLOW state.
    if (mSavedLocation != null)
    {
      notifyLocationUpdated();
      Logger.d(TAG, "Current location is available, so play the nice zoom animation");
      Framework.nativeRunFirstLaunchAnimation();
      return;
    }

    // Restart location service to show alert dialog if any location error.
    restart();
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

  public interface UiCallback
  {
    Activity requireActivity();
    void onMyPositionModeChanged(int newMode);
    void onLocationUpdated(@NonNull Location location);
    void onCompassUpdated(@NonNull CompassData compass);
    void onLocationError(int errorCode);
    void onLocationNotFound();
    void onRoutingFinish();
  }
}
