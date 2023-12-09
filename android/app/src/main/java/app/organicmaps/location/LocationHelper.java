package app.organicmaps.location;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.location.LocationManager.GPS_PROVIDER;
import static android.location.LocationManager.NETWORK_PROVIDER;
import static app.organicmaps.util.LocationUtils.FUSED_PROVIDER;
import static app.organicmaps.util.concurrency.UiThread.runLater;

import android.app.PendingIntent;
import android.content.Context;
import android.location.Location;
import android.location.LocationManager;
import android.os.Handler;
import android.os.Looper;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.annotation.UiThread;
import androidx.core.content.ContextCompat;
import androidx.core.location.GnssStatusCompat;
import androidx.core.location.LocationListenerCompat;
import androidx.core.location.LocationManagerCompat;
import androidx.core.location.LocationRequestCompat;

import app.organicmaps.Framework;
import app.organicmaps.Map;
import app.organicmaps.MwmApplication;
import app.organicmaps.bookmarks.data.FeatureId;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.util.Config;
import app.organicmaps.util.LocationUtils;
import app.organicmaps.util.NetworkPolicy;
import app.organicmaps.util.log.Logger;

import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.Objects;
import java.util.Set;

public class LocationHelper
{
  private static final long INTERVAL_FOLLOW_MS = 0;
  private static final long INTERVAL_NOT_FOLLOW_MS = 3000;
  private static final long INTERVAL_NAVIGATION_MS = 0;

  private static final long AGPS_EXPIRATION_TIME_MS = 16 * 60 * 60 * 1000; // 16 hours
  private static final long NOT_SWITCH_TO_NETWORK_WHEN_GPS_LOST_MS = 12000;

  @NonNull
  private final Context mContext;

  private static final String TAG = LocationState.LOCATION_TAG;

  private static final Looper sMainLooper = Objects.requireNonNull(Looper.getMainLooper());
  private static final Handler sHandler = new Handler(sMainLooper);

  @NonNull
  private final Set<LocationListener> mListeners = new LinkedHashSet<>();
  @Nullable
  private Location mSavedLocation;
  private MapObject mMyPosition;
  @NonNull
  private LocationRequestCompat mLocationRequest;
  private boolean mInFirstRun;
  @NonNull
  private final HashMap<String, LocationProvider> mProviders = new HashMap<>();
  @NonNull
  private final LocationManager mLocationManager;
  private @Nullable Runnable mGpsWatchdog;

  private class LocationListenerImpl implements LocationListenerCompat
  {
    @Override
    @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
    public void onLocationChanged(@NonNull Location location)
    {
      LocationHelper.this.onLocationChanged(location);
    }

    @Override
    @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
    public void onProviderEnabled(@NonNull String name)
    {
      final LocationProvider provider = mProviders.get(name);
      if (provider == null)
        throw new AssertionError("Unknown provider '" + name + "'");
      LocationHelper.this.enableProvider(name, provider);
    }

    @Override
    @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
    public void onProviderDisabled(@NonNull String name)
    {
      final LocationProvider provider = mProviders.get(name);
      if (provider == null)
        throw new AssertionError("Unknown provider '" + name + "'");
      LocationHelper.this.disableProvider(name, provider);
      checkStatus();
    }
  }

  @NonNull
  private GnssStatusCompat.Callback mGnssStatusCallback = new GnssStatusCompat.Callback()
  {
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

  @NonNull
  public static LocationHelper from(@NonNull Context context)
  {
    return MwmApplication.from(context).getLocationHelper();
  }

  public LocationHelper(@NonNull Context context)
  {
    mContext = context;
    mLocationManager = (LocationManager) MwmApplication.from(context).getSystemService(Context.LOCATION_SERVICE);
    // This service is always available on all versions of Android
    if (mLocationManager == null)
      throw new AssertionError("Can't get LOCATION_SERVICE");
    mLocationRequest = new LocationRequestCompat.Builder(0).build(); // stub

    for (String name: mLocationManager.getAllProviders())
    {
      if (FUSED_PROVIDER.equals(name))
        continue;
      mProviders.put(name, new AndroidNativeProvider(mLocationManager, name, new LocationListenerImpl()));
    }
    LocationProvider fusedProvider = LocationProviderFactory.getProvider(context, new LocationListenerImpl());
    if (fusedProvider != null)
      mProviders.put(FUSED_PROVIDER, fusedProvider);
  }

  /**
   * @return MapObject.MY_POSITION, null if location is not yet determined or "My position" button is switched off.
   */
  @Nullable
  public MapObject getMyPosition()
  {
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

  private void notifyLocationUpdated()
  {
    if (mSavedLocation == null)
      throw new IllegalStateException("No saved location");

    for (LocationListener listener : mListeners)
      listener.onLocationUpdated(mSavedLocation);

    // If we are still in the first run mode, i.e. user is staying on the first run screens,
    // not on the map, we mustn't post location update to the core. Only this preserving allows us
    // to play nice zoom animation once a user will leave first screens and will see a map.
    if (mInFirstRun)
    {
      Logger.d(TAG, "Location update is obtained and must be ignored, because the app is in a first run mode");
      return;
    }

    LocationState.nativeLocationUpdated(mSavedLocation.getTime(),
        mSavedLocation.getLatitude(),
        mSavedLocation.getLongitude(),
        mSavedLocation.getAccuracy(),
        mSavedLocation.getAltitude(),
        mSavedLocation.getSpeed(),
        mSavedLocation.getBearing());
  }

  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  private void onLocationChanged(@NonNull Location location)
  {
    Logger.d(TAG, "location = " + location);

    if (!LocationUtils.isAccuracySatisfied(location))
    {
      Logger.w(TAG, "Unsatisfied accuracy for location = " + location);
      return;
    }

    if (mSavedLocation != null)
    {
      if (!LocationUtils.isFromFusedProvider(location) && !LocationUtils.isLocationBetterThanLast(location, mSavedLocation))
      {
        Logger.d(TAG, "The new " + location + " is worse than the last " + mSavedLocation);
        return;
      }
    }

    mSavedLocation = location;
    mMyPosition = null;
    notifyLocationUpdated();

    if (mGpsWatchdog == null && LocationUtils.isFromGpsProvider(location) && RoutingController.get().isVehicleNavigation())
    {
      Logger.d(TAG, "Disabling all providers except 'gps' provider");
      for (var entry: mProviders.entrySet())
      {
        if (GPS_PROVIDER.equals(entry.getKey()))
          continue;
        disableProvider(entry.getKey(), entry.getValue());
      }
      mGpsWatchdog = this::checkGps;
      sHandler.postDelayed(mGpsWatchdog, NOT_SWITCH_TO_NETWORK_WHEN_GPS_LOST_MS);
    }
  }

  /**
   * Registers listener to obtain location updates.
   *
   * @param listener    listener to be registered.
   */
  @UiThread
  public void addListener(@NonNull LocationListener listener)
  {
    Logger.d(TAG, "listener: " + listener + " count was: " + mListeners.size());

    mListeners.add(listener);
    if (mSavedLocation != null)
      listener.onLocationUpdated(mSavedLocation);
  }

  /**
   * Removes given location listener.
   * @param listener listener to unregister.
   */
  @UiThread
  public void removeListener(@NonNull LocationListener listener)
  {
    Logger.d(TAG, "listener: " + listener + " count was: " + mListeners.size());
    mListeners.remove(listener);
  }

  private long calcLocationUpdatesInterval()
  {
    if (RoutingController.get().isNavigating())
      return INTERVAL_NAVIGATION_MS;

    final int mode = Map.isEngineCreated() ? LocationState.getMode() : LocationState.NOT_FOLLOW_NO_POSITION;
    switch (mode)
    {
      case LocationState.PENDING_POSITION:
      case LocationState.FOLLOW:
      case LocationState.FOLLOW_AND_ROTATE:
        return INTERVAL_FOLLOW_MS;
      case LocationState.NOT_FOLLOW:
      case LocationState.NOT_FOLLOW_NO_POSITION:
        return INTERVAL_NOT_FOLLOW_MS;
      default:
        throw new IllegalArgumentException("Unsupported location mode: " + mode);
    }
  }

  /**
   * Restart the location with a new refresh interval if changed.
   */
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  public void start()
  {
    final LocationRequestCompat newRequest = new LocationRequestCompat.Builder(calcLocationUpdatesInterval())
        // The quality is a hint to providers on how they should weigh power vs accuracy tradeoffs.
        .setQuality(LocationRequestCompat.QUALITY_HIGH_ACCURACY)
        .build();
    Logger.i(TAG, "old = " + mLocationRequest + " new = " + newRequest);
    if (!newRequest.equals(mLocationRequest))
    {
      for (var entry : mProviders.entrySet())
        disableProvider(entry.getKey(), entry.getValue());
    }
    mLocationRequest = newRequest;
    for (var entry : mProviders.entrySet())
      enableProvider(entry.getKey(), entry.getValue());
  }

  /**
   * Stops the polling location updates.
   */
  public void stop()
  {
    if (LocationUtils.checkLocationPermission(mContext))
      throw new AssertionError("Missing location permissions");

    Logger.i(TAG);
    if (mGpsWatchdog != null)
    {
      sHandler.removeCallbacks(mGpsWatchdog);
      mGpsWatchdog = null;
    }
    for (var entry : mProviders.entrySet())
      disableProvider(entry.getKey(), entry.getValue());
  }

  /**
   * Resume location services when entering the foreground.
   */
  public void resumeLocationInForeground()
  {
    if (!Map.isEngineCreated())
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
    mLocationManager.sendExtraCommand(GPS_PROVIDER, "force_xtra_injection", null);
    mLocationManager.sendExtraCommand(GPS_PROVIDER, "force_time_injection", null);
  }

  private void subscribeToGnssStatusUpdates()
  {
    // Subscribe to the low-level GNSS status to keep the green dot location indicator always firing.
    // https://github.com/organicmaps/organicmaps/issues/5999#issuecomment-1793713369
    if (!LocationUtils.checkFineLocationPermission(mContext))
      return;
    final LocationManager locationManager = (LocationManager) mContext.getSystemService(Context.LOCATION_SERVICE);
    LocationManagerCompat.registerGnssStatusCallback(locationManager, sHandler, mGnssStatusCallback);
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

  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  private void enableProvider(@NonNull String name, @NonNull LocationProvider provider)
  {
    if (provider.isActive())
      return;
    provider.start(mLocationRequest);

    if (GPS_PROVIDER.equals(name))
    {
      checkForAgpsUpdates();
      subscribeToGnssStatusUpdates();
      SensorHelper.from(mContext).start();
    }
  }

  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  private void disableProvider(@NonNull String name, @NonNull LocationProvider provider)
  {
    provider.stop();
    if (GPS_PROVIDER.equals(name))
    {
      unsubscribeFromGnssStatusUpdates();
      SensorHelper.from(mContext).stop();
    }
  }

  private void checkStatus()
  {
    PendingIntent resolutionIntent = null;
    for (LocationProvider provider: mProviders.values())
    {
      if (provider.isActive())
        return;
      if (resolutionIntent == null)
        resolutionIntent = provider.getResolutionIntent();
    }
    mMyPosition = null;
    for (LocationListener listener : mListeners)
      listener.onLocationDisabled(resolutionIntent);
  }

  private void checkGps()
  {
    if (!LocationUtils.checkLocationPermission(mContext))
      throw new AssertionError("Missing location permissions");
    if (mGpsWatchdog == null)
      throw new AssertionError("mGpsWatchdog is null");

    if (mSavedLocation != null && (System.currentTimeMillis() - mSavedLocation.getTime() < NOT_SWITCH_TO_NETWORK_WHEN_GPS_LOST_MS))
    {
      // GPS is alive.
      runLater(mGpsWatchdog, NOT_SWITCH_TO_NETWORK_WHEN_GPS_LOST_MS);
      return;
    }

    // GPS is not alive - start all providers.
    mGpsWatchdog = null;
    for (var entry : mProviders.entrySet())
      enableProvider(entry.getKey(), entry.getValue());
  }
}

