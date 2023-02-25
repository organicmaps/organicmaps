package app.organicmaps.location;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;

import android.app.PendingIntent;
import android.content.Context;
import android.location.Location;
import android.location.LocationManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresPermission;
import androidx.annotation.UiThread;
import androidx.core.content.ContextCompat;

import app.organicmaps.Framework;
import app.organicmaps.base.Initializable;
import app.organicmaps.bookmarks.data.FeatureId;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.util.Config;
import app.organicmaps.util.LocationUtils;
import app.organicmaps.util.NetworkPolicy;
import app.organicmaps.util.log.Logger;

import java.util.LinkedHashSet;
import java.util.Set;

public enum LocationHelper implements Initializable<Context>, BaseLocationProvider.Listener
{
  INSTANCE;

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

  private static final String TAG = LocationState.LOCATION_TAG;
  @NonNull
  private final Set<LocationListener> mListeners = new LinkedHashSet<>();
  @Nullable
  private Location mSavedLocation;
  private MapObject mMyPosition;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private BaseLocationProvider mLocationProvider;
  private long mInterval;
  private boolean mInFirstRun;
  private boolean mActive;

  @Override
  public void initialize(@NonNull Context context)
  {
    mContext = context;
    mLocationProvider = LocationProviderFactory.getProvider(mContext, this);
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
  }

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

    for (LocationListener listener : mListeners)
      listener.onLocationResolutionRequired(pendingIntent);
  }

  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  @Override
  @UiThread
  public void onFusedLocationUnsupported()
  {
    // Try to downgrade to the native provider first and restart the service before notifying the user.
    Logger.d(TAG, "provider = " + mLocationProvider.getClass().getSimpleName() + " is not supported," +
        " downgrading to use native provider");
    mLocationProvider = new AndroidNativeProvider(mContext, this);
    restart();
  }

  @Override
  @UiThread
  public void onLocationDisabled()
  {
    Logger.d(TAG, "provider = " + mLocationProvider.getClass().getSimpleName() +
        " settings = " + LocationUtils.areLocationServicesTurnedOn(mContext));

    stop();
    LocationState.nativeOnLocationError(LocationState.ERROR_GPS_OFF);

    for (LocationListener listener : mListeners)
      listener.onLocationDisabled();
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

  private void calcLocationUpdatesInterval()
  {
    if (RoutingController.get().isNavigating())
    {
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

      Logger.d(TAG, "navigation = " + router + " interval = " + mInterval);
      return;
    }

    int mode = LocationState.nativeGetMode();
    switch (mode)
    {
      case LocationState.FOLLOW:
        mInterval = INTERVAL_FOLLOW_MS;
        break;
      case LocationState.FOLLOW_AND_ROTATE:
        mInterval = INTERVAL_FOLLOW_AND_ROTATE_MS;
        break;
      case LocationState.NOT_FOLLOW:
      case LocationState.NOT_FOLLOW_NO_POSITION:
      case LocationState.PENDING_POSITION:
        mInterval = INTERVAL_NOT_FOLLOW_MS;
        break;
    }
    Logger.d(TAG, "mode = " + mode + " interval = " + mInterval);
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
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  public void restart()
  {
    Logger.d(TAG);
    stop();
    start();
  }

  /**
   * Starts polling location updates.
   */
  @RequiresPermission(anyOf = {ACCESS_COARSE_LOCATION, ACCESS_FINE_LOCATION})
  public void start()
  {
    if (isActive())
    {
      Logger.d(TAG, "Already started");
      return;
    }

    Logger.i(TAG);
    checkForAgpsUpdates();

    if (ContextCompat.checkSelfPermission(mContext, ACCESS_FINE_LOCATION) == PERMISSION_GRANTED)
      SensorHelper.from(mContext).start();

    final long oldInterval = mInterval;
    calcLocationUpdatesInterval();
    Logger.i(TAG, "provider = " + mLocationProvider.getClass().getSimpleName() +
        " mInFirstRun = " + mInFirstRun + " oldInterval = " + oldInterval + " interval = " + mInterval);
    mActive = true;
    mLocationProvider.start(mInterval);
  }

  /**
   * Stops the polling location updates.
   */
  public void stop()
  {
    if (!isActive())
    {
      Logger.d(TAG, "Already stopped");
      return;
    }

    Logger.i(TAG);
    mLocationProvider.stop();
    SensorHelper.from(mContext).stop();
    mActive = false;
  }

  /**
   * Resume location services when entering the foreground.
   */
  public void resumeLocationInForeground()
  {
    if (isActive())
      return;
    else if (LocationState.nativeGetMode() == LocationState.NOT_FOLLOW_NO_POSITION)
    {
      Logger.i(TAG, "Location updates are stopped by the user manually.");
      return;
    }
    else if (ContextCompat.checkSelfPermission(mContext, ACCESS_FINE_LOCATION) != PERMISSION_GRANTED &&
             ContextCompat.checkSelfPermission(mContext, ACCESS_COARSE_LOCATION) != PERMISSION_GRANTED)
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
}
