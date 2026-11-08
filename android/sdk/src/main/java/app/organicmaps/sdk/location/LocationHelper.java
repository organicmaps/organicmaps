package app.organicmaps.sdk.location;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.annotation.SuppressLint;
import android.app.PendingIntent;
import android.content.Context;
import android.location.Location;
import android.location.LocationManager;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.annotation.UiThread;
import androidx.core.content.ContextCompat;
import androidx.core.location.GnssStatusCompat;
import androidx.core.location.LocationManagerCompat;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.JunctionInfo;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.sdk.util.NetworkPolicy;
import app.organicmaps.sdk.util.log.Logger;
import java.util.concurrent.TimeUnit;
import org.chromium.base.ObserverList;

/**
 * Process-wide Android location and compass integration, obtained from {@link app.organicmaps.sdk.OrganicMaps}.
 * Call from the main thread. The host owns runtime location permission requests and the policy for starting
 * and stopping polling. A background navigation host also owns its foreground service and notification.
 */
public class LocationHelper implements BaseLocationProvider.Listener
{
  // Replaced 0 with 100ms. Seems like it is not working on Lineage, EOS, MicroG, ...
  // https://github.com/organicmaps/organicmaps/issues/11076
  // Probably https://github.com/organicmaps/organicmaps/issues/10133
  // Probably https://github.com/organicmaps/organicmaps/issues/9018
  private static final long INTERVAL_FOLLOW_MS = 100;
  private static final long INTERVAL_NOT_FOLLOW_MS = 3000;
  private static final long INTERVAL_NAVIGATION_MS = 100;
  private static final long INTERVAL_TRACK_RECORDING = 100;

  private static final long AGPS_EXPIRATION_TIME_MS = 16 * 60 * 60 * 1000; // 16 hours
  private static final long LOCATION_UPDATE_TIMEOUT_MS = 30 * 1000; // 30 seconds
  private static final long FUSED_TASK_TIMEOUT_MS = 5000;
  private static final long FUSED_FIRST_FIX_TIMEOUT_MS = 15_000;
  private static final long FUSED_STARTUP_TIMEOUT_MS = 2 * FUSED_TASK_TIMEOUT_MS + FUSED_FIRST_FIX_TIMEOUT_MS;

  @NonNull
  private final Context mContext;
  @NonNull
  private final SensorHelper mSensorHelper;

  private static final String TAG = LocationState.LOCATION_TAG;

  private final ObserverList<LocationListener> mListeners = new ObserverList<>();
  private final ObserverList.RewindableIterator<LocationListener> mListenersIterator = mListeners.rewindableIterator();

  @Nullable
  private Location mSavedLocation;
  @Nullable
  private Location mLocationForFiltering;
  private MapObject mMyPosition;
  @NonNull
  private final LocationProviderFactory mLocationProviderFactory;
  @NonNull
  private BaseLocationProvider mLocationProvider;
  private long mProviderGeneration;
  private boolean mSimulating;
  private long mInterval;
  private boolean mInFirstRun;
  private boolean mActive;
  private boolean mNativeFallback;
  private long mFusedAcquisitionStartedNanos = -1;
  private long mLastFreshFixNanos = -1;
  @Nullable
  private Runnable mFusedWatchdog;
  private final Handler mHandler;
  private final Runnable mLocationTimeoutRunnable = this::notifyLocationUpdateTimeout;

  @NonNull
  private final GnssStatusCompat.Callback mGnssStatusCallback = new GnssStatusCompat.Callback() {
    @Override
    public void onStarted()
    {
      Logger.d(TAG);
    }

    @Override
    public void onStopped()
    {
      Logger.d(TAG);
    }

    @Override
    public void onFirstFix(int ttffMillis)
    {
      Logger.d(TAG, "ttffMillis = " + ttffMillis);
    }

    @Override
    public void onSatelliteStatusChanged(@NonNull GnssStatusCompat status)
    {
      int used = 0;
      boolean fixed = false;
      for (int i = 0; i < status.getSatelliteCount(); i++)
      {
        if (status.usedInFix(i))
        {
          used++;
          fixed = true;
        }
      }
      Logger.d(TAG, "total = " + status.getSatelliteCount() + " used = " + used + " fixed = " + fixed);
    }
  };

  public LocationHelper(@NonNull Context context, @NonNull SensorHelper sensorHelper)
  {
    this(context, sensorHelper, new LocationProviderFactory(), new Handler(Looper.getMainLooper()));
  }

  LocationHelper(@NonNull Context context, @NonNull SensorHelper sensorHelper,
                 @NonNull LocationProviderFactory providerFactory, @NonNull Handler handler)
  {
    mContext = context;
    mSensorHelper = sensorHelper;
    mLocationProviderFactory = providerFactory;
    mHandler = handler;
    mLocationProvider = createProvider(false, createProviderListener());
  }

  private BaseLocationProvider.Listener createProviderListener()
  {
    clearFusedWatchdog();
    final long generation = ++mProviderGeneration;
    mLocationForFiltering = null;
    return new BaseLocationProvider.Listener() {
      private boolean isCurrent()
      {
        return mActive && generation == mProviderGeneration;
      }

      @Override
      public void onLocationChanged(@NonNull Location location)
      {
        if (isCurrent())
          LocationHelper.this.onLocationChanged(location);
      }

      @Override
      public void onLocationDisabled()
      {
        if (isCurrent())
          LocationHelper.this.onLocationDisabled();
      }

      @Override
      public void onLocationPermissionDenied()
      {
        if (isCurrent())
          LocationHelper.this.onLocationPermissionDenied();
      }

      @Override
      public void onLocationResolutionRequired(@NonNull PendingIntent pendingIntent)
      {
        if (isCurrent())
          LocationHelper.this.onLocationResolutionRequired(pendingIntent);
      }

      @Override
      public void onFusedLocationUnsupported()
      {
        if (isCurrent())
          LocationHelper.this.onFusedLocationUnsupported();
      }

      @Override
      public void onFusedLocationProgress(@NonNull BaseLocationProvider.StartupPhase phase)
      {
        if (isCurrent())
          LocationHelper.this.onFusedLocationProgress(phase);
      }
    };
  }

  private BaseLocationProvider createProvider(boolean nativeOnly, @NonNull BaseLocationProvider.Listener listener)
  {
    return nativeOnly ? mLocationProviderFactory.getNativeProvider(mContext, listener)
                      : mLocationProviderFactory.getProvider(mContext, listener);
  }

  // Permission is checked before restarting; an inactive replacement needs none.
  @SuppressLint("MissingPermission")
  private void replaceProvider(boolean nativeOnly)
  {
    mNativeFallback = nativeOnly;
    final BaseLocationProvider.Listener listener = createProviderListener();
    if (mActive)
      mLocationProvider.stop();
    mLocationProvider = createProvider(nativeOnly, listener);
    if (mActive)
    {
      if (LocationUtils.checkLocationPermission(mContext))
      {
        mHandler.removeCallbacks(mLocationTimeoutRunnable);
        mHandler.postDelayed(mLocationTimeoutRunnable, LOCATION_UPDATE_TIMEOUT_MS);
        mLocationProvider.start(mInterval);
      }
      else
        onLocationPermissionDenied();
    }
  }

  /** Applies the location backend preference without starting inactive acquisition or interrupting simulation. */
  @UiThread
  public void reloadProvider()
  {
    if (!mSimulating)
      replaceProvider(false);
  }

  private long fusedFixTimeoutNanos()
  {
    return TimeUnit.MILLISECONDS.toNanos(Math.max(FUSED_TASK_TIMEOUT_MS, 3 * mInterval));
  }

  private void clearFusedWatchdog()
  {
    if (mFusedWatchdog != null)
      mHandler.removeCallbacks(mFusedWatchdog);
    mFusedWatchdog = null;
    mFusedAcquisitionStartedNanos = -1;
    mLastFreshFixNanos = -1;
  }

  private void watchFusedDeadline(@NonNull String reason, long deadline)
  {
    if (mFusedWatchdog != null)
      mHandler.removeCallbacks(mFusedWatchdog);
    final long generation = mProviderGeneration;
    mFusedWatchdog = new Runnable() {
      @Override
      public void run()
      {
        if (mFusedWatchdog != this || !mActive || mSimulating || generation != mProviderGeneration)
          return;
        mHandler.removeCallbacks(this);
        final long remaining = deadline - SystemClock.elapsedRealtimeNanos();
        if (remaining > 0)
        {
          mHandler.postDelayed(this, TimeUnit.NANOSECONDS.toMillis(remaining) + 1);
          return;
        }
        mFusedWatchdog = null;
        Logger.w(TAG, "Fused location deadline expired: " + reason + " generation = " + generation);
        onFusedLocationUnsupported();
      }
    };
    mHandler.postDelayed(mFusedWatchdog,
                         Math.max(0, TimeUnit.NANOSECONDS.toMillis(deadline - SystemClock.elapsedRealtimeNanos()) + 1));
  }

  @Override
  @UiThread
  public void onFusedLocationProgress(@NonNull BaseLocationProvider.StartupPhase phase)
  {
    if (!mActive || mSimulating || mNativeFallback)
      return;
    final long now = SystemClock.elapsedRealtimeNanos();
    if (mFusedAcquisitionStartedNanos < 0)
      mFusedAcquisitionStartedNanos = now;
    Logger.d(TAG, "Fused location phase = " + phase + " generation = " + mProviderGeneration);
    if (mLastFreshFixNanos >= 0)
    {
      watchFusedDeadline("fresh fixes stopped", mLastFreshFixNanos + fusedFixTimeoutNanos());
      return;
    }
    final long timeout =
        phase == BaseLocationProvider.StartupPhase.REGISTERED ? FUSED_FIRST_FIX_TIMEOUT_MS : FUSED_TASK_TIMEOUT_MS;
    final long deadline =
        Math.min(now + TimeUnit.MILLISECONDS.toNanos(timeout),
                 mFusedAcquisitionStartedNanos + TimeUnit.MILLISECONDS.toNanos(FUSED_STARTUP_TIMEOUT_MS));
    watchFusedDeadline(phase.name(), deadline);
  }

  private void updateFusedFreshness(@NonNull Location location)
  {
    if (mSimulating)
      return;
    final long timestamp = location.getElapsedRealtimeNanos();
    final long age = SystemClock.elapsedRealtimeNanos() - timestamp;
    if (timestamp <= mLastFreshFixNanos || age < 0 || age > fusedFixTimeoutNanos())
      return;
    mLastFreshFixNanos = timestamp;
    if (mFusedAcquisitionStartedNanos >= 0)
    {
      Logger.d(TAG, "Fused fix time = " + location.getTime() + " elapsed nanos = " + timestamp
                        + " age millis = " + TimeUnit.NANOSECONDS.toMillis(age));
      watchFusedDeadline("fresh fixes stopped", timestamp + fusedFixTimeoutNanos());
    }
  }

  /**
   * @return MapObject.MY_POSITION, null if location is not yet determined or "My position" button is switched off.
   */
  @Nullable
  public MapObject getMyPosition()
  {
    if (!isActive())
    {
      mMyPosition = null;
      return null;
    }

    if (mSavedLocation == null)
      return null;

    if (mMyPosition == null)
      mMyPosition = MapObject.createMapObject(MapObject.MY_POSITION, "", "", mSavedLocation.getLatitude(),
                                              mSavedLocation.getLongitude());

    return mMyPosition;
  }

  /**
   * Obtains last known location.
   * @return {@code null} if no location is saved.
   */
  @Nullable
  public Location getSavedLocation()
  {
    return mSavedLocation;
  }

  /**
   * Indicates about whether a location provider is polling location updates right now or not.
   */
  public boolean isActive()
  {
    return mActive;
  }

  private void notifyLocationUpdated()
  {
    if (mSavedLocation == null)
      throw new IllegalStateException("No saved location");

    mHandler.removeCallbacks(mLocationTimeoutRunnable);
    mHandler.postDelayed(mLocationTimeoutRunnable, LOCATION_UPDATE_TIMEOUT_MS); // Reset the timeout.

    mListenersIterator.rewind();
    while (mListenersIterator.hasNext())
      mListenersIterator.next().onLocationUpdated(mSavedLocation);

    // If we are still in the first run mode, i.e. user is staying on the first run screens,
    // not on the map, we mustn't post location update to the core. Only this preserving allows us
    // to play nice zoom animation once a user will leave first screens and will see a map.
    if (mInFirstRun)
    {
      Logger.d(TAG, "Location update is obtained and must be ignored, because the app is in a first run mode");
      return;
    }

    final LocationCompatExtractor.Altitude altitude = LocationCompatExtractor.getAltitude(mSavedLocation);
    LocationState.nativeLocationUpdated(
        mSavedLocation.getTime(), mSavedLocation.getLatitude(), mSavedLocation.getLongitude(),
        mSavedLocation.getAccuracy(), altitude != null ? altitude.altitude() : 0,
        altitude != null ? altitude.accuracy() : -1, mSavedLocation.hasSpeed() ? mSavedLocation.getSpeed() : -1,
        mSavedLocation.hasBearing() ? mSavedLocation.getBearing() : -1);
  }

  private void notifyLocationUpdateTimeout()
  {
    mHandler.removeCallbacks(mLocationTimeoutRunnable);
    if (!isActive())
    {
      Logger.w(TAG, "Provider is not active");
      return;
    }

    Logger.d(TAG);
    notifyLocationTimeoutListeners();
  }

  private void notifyLocationTimeoutListeners()
  {
    mListenersIterator.rewind();
    while (mListenersIterator.hasNext())
      mListenersIterator.next().onLocationUpdateTimeout();
  }

  @Override
  public void onLocationChanged(@NonNull Location location)
  {
    Logger.d(TAG, "provider = " + mLocationProvider.getClass().getSimpleName() + " location = " + location);

    if (!isActive())
    {
      Logger.w(TAG, "Provider is not active");
      return;
    }

    if (!LocationUtils.isAccuracySatisfied(location))
    {
      Logger.w(TAG, "Unsatisfied accuracy for location = " + location);
      return;
    }

    // Fresh valid output proves backend liveness even when the display retains a more accurate fix.
    updateFusedFreshness(location);

    if (mLocationForFiltering != null)
    {
      if (!LocationUtils.isLocationBetterThanLast(location, mLocationForFiltering))
      {
        Logger.d(TAG, "The new " + location + " is worse than the last " + mLocationForFiltering);
        return;
      }
    }

    mLocationForFiltering = location;
    mSavedLocation = location;
    mMyPosition = null;
    notifyLocationUpdated();
  }

  // Used by GoogleFusedLocationProvider.
  @SuppressWarnings("unused")
  @Override
  @UiThread
  public void onLocationResolutionRequired(@NonNull PendingIntent pendingIntent)
  {
    Logger.d(TAG);

    if (!isActive())
    {
      Logger.w(TAG, "Provider is not active");
      return;
    }

    // Stop provider until location resolution is granted.
    stop();
    LocationState.nativeOnLocationError(LocationState.ERROR_GPS_OFF);

    mListenersIterator.rewind();
    while (mListenersIterator.hasNext())
      mListenersIterator.next().onLocationResolutionRequired(pendingIntent);
  }

  // Used by GoogleFusedLocationProvider.
  @SuppressWarnings("unused")
  @Override
  @UiThread
  public void onFusedLocationUnsupported()
  {
    if (!mActive || mNativeFallback)
      return;
    if (!LocationUtils.checkLocationPermission(mContext))
    {
      onLocationPermissionDenied();
      return;
    }
    if (!LocationUtils.areLocationServicesTurnedOn(mContext))
    {
      onLocationDisabled();
      return;
    }
    // Try to downgrade to the native provider first and restart the service before notifying the user.
    Logger.d(TAG, "provider = " + mLocationProvider.getClass().getSimpleName() + " is not supported,"
                      + " downgrading to use native provider");
    replaceProvider(true);
  }

  // RouteSimulationProvider doesn't really require location permissions.
  @SuppressLint("MissingPermission")
  public void startNavigationSimulation(JunctionInfo[] points)
  {
    Logger.i(TAG);
    if (mSimulating)
      throw new IllegalStateException("Simulation is already started");
    final BaseLocationProvider.Listener listener = createProviderListener();
    if (mActive)
      mLocationProvider.stop();
    mSimulating = true;
    mLocationProvider = new RouteSimulationProvider(mContext, listener, points);
    mActive = true;
    mLocationProvider.start(mInterval);
  }

  @SuppressLint("MissingPermission")
  public void stopNavigationSimulation()
  {
    Logger.i(TAG);
    if (!mSimulating)
      throw new IllegalStateException("Should be called only after startNavigationSimulation()");
    mSimulating = false;
    replaceProvider(false);
  }

  @Override
  @UiThread
  public void onLocationPermissionDenied()
  {
    if (!mActive)
      return;
    stop();
    LocationState.nativeOnLocationError(LocationState.ERROR_DENIED);
  }

  @Override
  @UiThread
  public void onLocationDisabled()
  {
    if (!mActive)
      return;
    if (!LocationUtils.checkLocationPermission(mContext))
    {
      onLocationPermissionDenied();
      return;
    }
    Logger.d(TAG, "provider = " + mLocationProvider.getClass().getSimpleName()
                      + " settings = " + LocationUtils.areLocationServicesTurnedOn(mContext));

    if (LocationUtils.areLocationServicesTurnedOn(mContext))
    {
      stop();
      LocationState.nativeOnLocationError(LocationState.ERROR_NOT_SUPPORTED);
      notifyLocationTimeoutListeners();
      return;
    }

    stop();
    LocationState.nativeOnLocationError(LocationState.ERROR_GPS_OFF);

    mListenersIterator.rewind();
    while (mListenersIterator.hasNext())
      mListenersIterator.next().onLocationDisabled();
  }

  /**
   * Registers a main-thread location listener and immediately replays a saved location if one is available.
   * Remove the listener when its host ends.
   */
  @UiThread
  public void addListener(@NonNull LocationListener listener)
  {
    Logger.d(TAG, "listener: " + listener + " count was: " + mListeners.size());

    mListeners.addObserver(listener);
    if (mSavedLocation != null)
      listener.onLocationUpdated(mSavedLocation);
  }

  @UiThread
  public void removeListener(@NonNull LocationListener listener)
  {
    Logger.d(TAG, "listener: " + listener + " count was: " + mListeners.size());
    mListeners.removeObserver(listener);
  }

  private long calcLocationUpdatesInterval()
  {
    if (RoutingController.get().isNavigating())
      return INTERVAL_NAVIGATION_MS;

    if (TrackRecorder.nativeIsTrackRecordingEnabled())
      return INTERVAL_TRACK_RECORDING;

    final int mode = Map.isEngineCreated() ? LocationState.getMode() : LocationState.NOT_FOLLOW_NO_POSITION;
    return switch (mode)
    {
      case LocationState.PENDING_POSITION, LocationState.FOLLOW, LocationState.FOLLOW_AND_ROTATE -> INTERVAL_FOLLOW_MS;
      case LocationState.NOT_FOLLOW, LocationState.NOT_FOLLOW_NO_POSITION -> INTERVAL_NOT_FOLLOW_MS;
      default -> throw new IllegalArgumentException("Unsupported location mode: " + mode);
    };
  }

  /**
   * Starts location polling or updates its interval for the current position, recording, and navigation mode.
   * The host must have a coarse or fine location permission before calling.
   */
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  public void restartWithNewMode()
  {
    if (!isActive())
    {
      start();
      return;
    }

    final long newInterval = calcLocationUpdatesInterval();
    if (newInterval == mInterval)
      return;

    Logger.i(TAG, "update refresh interval: old = " + mInterval + " new = " + newInterval);
    mLocationProvider.stop();
    mInterval = newInterval;
    mLocationProvider.start(newInterval);
  }

  /**
   * Starts polling and, with fine permission, compass/GNSS updates. Repeated calls while active do nothing.
   * The host must already hold a coarse or fine location permission.
   */
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  public void start()
  {
    if (isActive())
    {
      Logger.d(TAG, "Already started");
      return;
    }

    if (mNativeFallback && !mSimulating)
      replaceProvider(false);

    Logger.i(TAG);
    checkForAgpsUpdates();

    if (LocationUtils.checkFineLocationPermission(mContext))
      mSensorHelper.start();

    final long oldInterval = mInterval;
    mInterval = calcLocationUpdatesInterval();
    Logger.i(TAG, "provider = " + mLocationProvider.getClass().getSimpleName() + " mInFirstRun = " + mInFirstRun
                      + " oldInterval = " + oldInterval + " interval = " + mInterval);
    mActive = true;
    mLocationProvider.start(mInterval);
    mHandler.postDelayed(mLocationTimeoutRunnable, LOCATION_UPDATE_TIMEOUT_MS);
    subscribeToGnssStatusUpdates();
  }

  /**
   * Stops polling, sensors, and the location timeout. Repeated calls while inactive do nothing.
   */
  public void stop()
  {
    if (!isActive())
    {
      Logger.d(TAG, "Already stopped");
      return;
    }

    Logger.i(TAG);
    mActive = false;
    clearFusedWatchdog();
    mLocationProvider.stop();
    unsubscribeFromGnssStatusUpdates();
    mSensorHelper.stop();
    mHandler.removeCallbacks(mLocationTimeoutRunnable);
  }

  /**
   * Resumes polling when the renderer exists, permission is granted, and the position mode allows it.
   * Safe to call when entering the foreground; it does nothing if these prerequisites are absent.
   */
  public void resumeLocationInForeground()
  {
    if (isActive())
    {
      if (!mSimulating && !LocationUtils.checkLocationPermission(mContext))
        onLocationPermissionDenied();
      else if (mFusedWatchdog != null)
        mFusedWatchdog.run();
      return;
    }
    else if (!Map.isEngineCreated())
    {
      // LocationState.nativeGetMode() is initialized only after drape creation.
      // https://github.com/organicmaps/organicmaps/issues/1128#issuecomment-1784435190
      Logger.d(TAG, "Engine is not created yet.");
      return;
    }
    else if (LocationState.getMode() == LocationState.NOT_FOLLOW_NO_POSITION)
    {
      Logger.i(TAG, "Location updates are stopped by the user manually.");
      return;
    }
    else if (!LocationUtils.checkLocationPermission(mContext))
    {
      Logger.i(TAG, "Permissions ACCESS_FINE_LOCATION and ACCESS_COARSE_LOCATION are not granted");
      return;
    }

    start();
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

  private void subscribeToGnssStatusUpdates()
  {
    // Subscribe to the low-level GNSS status to keep the green dot location indicator always firing.
    // https://github.com/organicmaps/organicmaps/issues/5999#issuecomment-1793713369
    if (!LocationUtils.checkFineLocationPermission(mContext))
      return;
    final LocationManager locationManager = (LocationManager) mContext.getSystemService(Context.LOCATION_SERVICE);
    LocationManagerCompat.registerGnssStatusCallback(locationManager, ContextCompat.getMainExecutor(mContext),
                                                     mGnssStatusCallback);
  }

  private void unsubscribeFromGnssStatusUpdates()
  {
    final LocationManager locationManager = (LocationManager) mContext.getSystemService(Context.LOCATION_SERVICE);
    LocationManagerCompat.unregisterGnssStatusCallback(locationManager, mGnssStatusCallback);
  }

  @UiThread
  public boolean isInFirstRun()
  {
    return mInFirstRun;
  }

  @UiThread
  public void onEnteredIntoFirstRun()
  {
    Logger.i(TAG);
    mInFirstRun = true;
  }

  @UiThread
  public void onExitFromFirstRun()
  {
    Logger.i(TAG);
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
    }
  }

  public boolean isGmsLocationProviderAvailable()
  {
    return mLocationProviderFactory.isGmsLocationProviderAvailable(mContext);
  }
}
