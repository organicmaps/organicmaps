package app.organicmaps.location;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.app.Activity;
import android.app.Dialog;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.location.LocationManager;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.IntentSenderRequest;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import androidx.appcompat.app.AppCompatActivity;

import app.organicmaps.Framework;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.background.AppBackgroundTracker;
import app.organicmaps.base.Initializable;
import app.organicmaps.bookmarks.data.FeatureId;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.util.Config;
import app.organicmaps.util.Listeners;
import app.organicmaps.util.LocationUtils;
import app.organicmaps.util.NetworkPolicy;
import app.organicmaps.util.Utils;
import app.organicmaps.util.log.Logger;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

public enum LocationHelper implements Initializable<Context>, AppBackgroundTracker.OnTransitionListener, BaseLocationProvider.Listener
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

  private static final String TAG = LocationHelper.class.getSimpleName();
  @NonNull
  private final Listeners<LocationListener> mListeners = new Listeners<>();
  @Nullable
  private Location mSavedLocation;
  private double mSavedNorth = Double.NaN;
  private MapObject mMyPosition;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private SensorHelper mSensorHelper;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private BaseLocationProvider mLocationProvider;
  @Nullable
  private AppCompatActivity mActivity;
  private long mInterval;
  private boolean mInFirstRun;
  private boolean mActive;
  @Nullable
  private Dialog mErrorDialog;
  @Nullable
  private ActivityResultLauncher<String[]> mPermissionRequest;
  @Nullable
  private ActivityResultLauncher<IntentSenderRequest> mResolutionRequest;

  @Override
  public void initialize(@NonNull Context context)
  {
    mContext = context;
    mSensorHelper = new SensorHelper(context);
    mLocationProvider = LocationProviderFactory.getProvider(mContext, this);
    MwmApplication.backgroundTracker(context).addListener(this);
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

  @Override
  public void onTransit(boolean foreground)
  {
    Logger.d(TAG, "foreground = " + foreground + " mode = " + LocationState.nativeGetMode());

    if (foreground)
    {
      if (isActive())
        return;

      if (LocationState.nativeGetMode() == LocationState.NOT_FOLLOW_NO_POSITION)
      {
        Logger.d(TAG, "Location updates are stopped by the user manually, so skip provider start"
            + " until the user starts it manually.");
        return;
      }

      Logger.d(TAG, "Starting in foreground");
      start();
    }
    else
    {
      if (!isActive())
        return;

      Logger.d(TAG, "Stopping in background");
      stop();
    }
  }

  public void closeLocationDialog()
  {
    if (mErrorDialog != null && mErrorDialog.isShowing())
      mErrorDialog.dismiss();
    mErrorDialog = null;
  }

  void notifyCompassUpdated(double north)
  {
    mSavedNorth = north;
    if (mActivity != null)
    {
      int rotation = mActivity.getWindowManager().getDefaultDisplay().getRotation();
      mSavedNorth = LocationUtils.correctCompassAngle(rotation, mSavedNorth);
    }
    for (LocationListener listener : mListeners)
      listener.onCompassUpdated(mSavedNorth);
    mListeners.finishIterate();
  }

  private void notifyLocationUpdated()
  {
    if (mSavedLocation == null)
      throw new IllegalStateException("No saved location");

    closeLocationDialog();

    for (LocationListener listener : mListeners)
      listener.onLocationUpdated(mSavedLocation);
    mListeners.finishIterate();

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
    if (!isActive())
      return;

    Logger.d(TAG, "provider = " + mLocationProvider.getClass().getSimpleName() + " location = " + location);

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
    if (!isActive())
      return;

    Logger.d(TAG);

    if (mResolutionRequest == null)
    {
      Logger.d(TAG, "Can't resolve location permissions because UI is not attached");
      stop();
      LocationState.nativeOnLocationError(LocationState.ERROR_GPS_OFF);
      return;
    }

    // Cancel our dialog in favor of system dialog.
    closeLocationDialog();

    // Launch system permission resolution dialog.
    IntentSenderRequest intentSenderRequest = new IntentSenderRequest.Builder(pendingIntent.getIntentSender())
        .build();
    mResolutionRequest.launch(intentSenderRequest);
  }

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
        " permissions = " + LocationUtils.isLocationGranted(mContext) +
        " settings = " + LocationUtils.areLocationServicesTurnedOn(mContext));

    stop();
    LocationState.nativeOnLocationError(LocationState.ERROR_GPS_OFF);

    if (mActivity == null)
    {
      Logger.d(TAG, "Don't show 'location disabled' error dialog because Activity is not attached");
      return;
    }

    if (mErrorDialog != null && mErrorDialog.isShowing())
    {
      Logger.d(TAG, "Don't show 'location disabled' error dialog because another dialog is in progress");
      return;
    }

    MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(mActivity, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.enable_location_services)
        .setMessage(R.string.location_is_disabled_long_text)
        .setOnDismissListener(dialog -> mErrorDialog = null)
        .setNegativeButton(R.string.close, null);
    final Intent intent = Utils.makeSystemLocationSettingIntent(mActivity);
    if (intent != null)
    {
      intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
      intent.addFlags(Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
      builder.setPositiveButton(R.string.connection_settings, (dialog, which) -> mActivity.startActivity(intent));
    }
    mErrorDialog = builder.show();
  }

  @UiThread
  private void onLocationDenied()
  {
    Logger.d(TAG, "provider = " + mLocationProvider.getClass().getSimpleName() +
        " permissions = " + LocationUtils.isLocationGranted(mContext) +
        " settings = " + LocationUtils.areLocationServicesTurnedOn(mContext));

    stop();
    LocationState.nativeOnLocationError(LocationState.ERROR_DENIED);

    if (mActivity == null)
    {
      Logger.w(TAG, "Don't show 'location denied' error dialog because Activity is not attached");
      return;
    }

    if (mErrorDialog != null && mErrorDialog.isShowing())
    {
      Logger.w(TAG, "Don't show 'location denied' error dialog because another dialog is in progress");
      return;
    }

    mErrorDialog = new MaterialAlertDialogBuilder(mActivity, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.enable_location_services)
        .setMessage(R.string.location_is_disabled_long_text)
        .setOnDismissListener(dialog -> mErrorDialog = null)
        .setNegativeButton(R.string.close, null)
        .show();
  }

  @UiThread
  private void onLocationPendingTimeout()
  {
    Logger.d(TAG, " permissions = " + LocationUtils.isLocationGranted(mContext) +
        " settings = " + LocationUtils.areLocationServicesTurnedOn(mContext));

    //
    // For all cases below we don't stop location provider until user explicitly clicks "Stop" in the dialog.
    //

    if (!isActive())
    {
      Logger.d(TAG, "Don't show 'location timeout' error dialog because provider is already stopped");
      return;
    }

    if (mActivity == null)
    {
      Logger.d(TAG, "Don't show 'location timeout' error dialog because Activity is not attached");
      return;
    }

    if (mErrorDialog != null && mErrorDialog.isShowing())
    {
      Logger.d(TAG, "Don't show 'location timeout' error dialog because another dialog is in progress");
      return;
    }

    final AppCompatActivity activity = mActivity;
    mErrorDialog = new MaterialAlertDialogBuilder(activity, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.current_location_unknown_title)
        .setMessage(R.string.current_location_unknown_message)
        .setOnDismissListener(dialog -> mErrorDialog = null)
        .setNegativeButton(R.string.current_location_unknown_stop_button, (dialog, which) ->
        {
          Logger.w(TAG, "Disabled by user");
          LocationState.nativeOnLocationError(LocationState.ERROR_GPS_OFF);
          stop();
        })
        .setPositiveButton(R.string.current_location_unknown_continue_button, (dialog, which) ->
        {
          // Do nothing - provider will continue to search location.
        })
        .show();
  }

  /**
   * Registers listener to obtain location updates.
   *
   * @param listener    listener to be registered.
   */
  @UiThread
  public void addListener(@NonNull LocationListener listener)
  {
    Logger.d(TAG, "listener: " + listener + " count was: " + mListeners.getSize());

    mListeners.register(listener);
    if (mSavedLocation != null)
      listener.onLocationUpdated(mSavedLocation);
    if (!Double.isNaN(mSavedNorth))
      listener.onCompassUpdated(mSavedNorth);
  }

  @UiThread
  /**
   * Removes given location listener.
   * @param listener listener to unregister.
   */
  public void removeListener(@NonNull LocationListener listener)
  {
    Logger.d(TAG, "listener: " + listener + " count was: " + mListeners.getSize());
    mListeners.unregister(listener);
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
  public void restart()
  {
    Logger.d(TAG);
    stop();
    start();
  }

  /**
   * Starts polling location updates.
   */
  public void start()
  {
    Logger.d(TAG);

    if (isActive())
      throw new IllegalStateException("Already started");

    if (!LocationUtils.isLocationGranted(mContext))
    {
      Logger.w(TAG, "Dynamic permissions ACCESS_COARSE_LOCATION and/or ACCESS_FINE_LOCATION are not granted");
      Logger.d(TAG, "error mode = " + LocationState.nativeGetMode());
      LocationState.nativeOnLocationError(LocationState.ERROR_DENIED);

      if (mPermissionRequest == null)
      {
        Logger.w(TAG, "Don't request location permissions because Activity is not attached");
        return;
      }
      mPermissionRequest.launch(new String[]{
          ACCESS_COARSE_LOCATION,
          ACCESS_FINE_LOCATION
      });
      return;
    }

    checkForAgpsUpdates();

    LocationState.nativeSetLocationPendingTimeoutListener(this::onLocationPendingTimeout);
    mSensorHelper.start();
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
    Logger.d(TAG);

    if (!isActive())
    {
      Logger.w(TAG, "Already stopped");
      return;
    }

    mLocationProvider.stop();
    mSensorHelper.stop();
    LocationState.nativeRemoveLocationPendingTimeoutListener();
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
  public void attach(@NonNull AppCompatActivity activity)
  {
    Logger.d(TAG, "activity = " + activity);

    if (mActivity != null)
    {
      Logger.e(TAG, "Another Activity = " + mActivity + " is already attached");
      detach();
    }

    mActivity = activity;

    mPermissionRequest = mActivity.registerForActivityResult(new ActivityResultContracts.RequestMultiplePermissions(),
        result -> onRequestPermissionsResult());
    mResolutionRequest = mActivity.registerForActivityResult(new ActivityResultContracts.StartIntentSenderForResult(),
        result -> onLocationResolutionResult(result.getResultCode()));
  }

  /**
   * Detach UI from helper.
   */
  @UiThread
  public void detach()
  {
    Logger.d(TAG, "activity = " + mActivity);

    if (mActivity == null)
    {
      Logger.e(TAG, "Activity is not attached");
      return;
    }

    assert mPermissionRequest != null;
    mPermissionRequest.unregister();
    mPermissionRequest = null;
    assert mResolutionRequest != null;
    mResolutionRequest.unregister();
    mResolutionRequest = null;
    mActivity = null;
  }

  @UiThread
  private void onRequestPermissionsResult()
  {
    Logger.d(TAG);

    if (LocationUtils.isLocationGranted(mContext))
    {
      Logger.i(TAG, "Permissions have been granted");
      if (!isActive())
        start();
      return;
    }

    Logger.w(TAG, "Permissions have not been granted");
    onLocationDenied();
  }

  @UiThread
  private void onLocationResolutionResult(int resultCode)
  {
    if (resultCode != Activity.RESULT_OK)
    {
      Logger.w(TAG, "Resolution has not been granted");
      stop();
      LocationState.nativeOnLocationError(LocationState.ERROR_GPS_OFF);
      return;
    }

    Logger.i(TAG, "Resolution has been granted");
    restart();
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
      return;
    }

    // Restart location service to show alert dialog if any location error.
    restart();
  }

  public double getSavedNorth()
  {
    return mSavedNorth;
  }
}
