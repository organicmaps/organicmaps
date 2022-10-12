package com.mapswithme.maps.location;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;

import android.app.Activity;
import android.app.Dialog;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.location.Location;
import android.location.LocationManager;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.IntentSenderRequest;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.UiThread;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.background.AppBackgroundTracker;
import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Config;
import com.mapswithme.util.Listeners;
import com.mapswithme.util.LocationUtils;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;

public enum LocationHelper implements Initializable<Context>, AppBackgroundTracker.OnTransitionListener, BaseLocationProvider.Listener
{
  INSTANCE;

  // These constants should correspond to values defined in platform/location.hpp
  // Leave 0-value as no any error.
  //private static final int ERROR_UNKNOWN = 0;
  //private static final int ERROR_NOT_SUPPORTED = 1;
  private static final int ERROR_DENIED = 2;
  private static final int ERROR_GPS_OFF = 3;

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
  private boolean mErrorDialogAnnoying;
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

  public void switchToNextMode()
  {
    Logger.d(TAG);
    LocationState.nativeSwitchToNextMode();
  }

  /**
   * Indicates about whether a location provider is polling location updates right now or not.
   */
  public boolean isActive()
  {
    return mActive;
  }

  private void setStopLocationUpdateByUser(boolean isStopped)
  {
    Logger.d(TAG, "isStopped = " + isStopped);
    mLocationUpdateStoppedByUser = isStopped;
  }

  public boolean isLocationErrorDialogAnnoying()
  {
    return mErrorDialogAnnoying;
  }

  public void setLocationErrorDialogAnnoying(boolean isAnnoying)
  {
    Logger.d(TAG, "isAnnoying = " + isAnnoying);
    mErrorDialogAnnoying = isAnnoying;
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
    if (mCompassData == null)
      mCompassData = new CompassData();

    mCompassData.update(mContext, north);
    if (mUiCallback != null)
      mUiCallback.onCompassUpdated(mCompassData);

    for (LocationListener listener : mListeners)
      listener.onCompassUpdated(time, north);
    mListeners.finishIterate();
  }

  private void notifyLocationUpdated()
  {
    if (mSavedLocation == null)
      throw new IllegalStateException("No saved location");

    if (mErrorDialog != null && mErrorDialog.isShowing())
      mErrorDialog.dismiss();

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

    nativeLocationUpdated(mSavedLocation.getTime(),
        mSavedLocation.getLatitude(),
        mSavedLocation.getLongitude(),
        mSavedLocation.getAccuracy(),
        mSavedLocation.getAltitude(),
        mSavedLocation.getSpeed(),
        mSavedLocation.getBearing());

    if (mUiCallback != null)
      mUiCallback.onLocationUpdated(mSavedLocation);

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
    Logger.d(TAG, "location = " + location);

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
  @UiThread
  public void onLocationResolutionRequired(@Nullable PendingIntent pendingIntent)
  {
    Logger.d(TAG);

    if (mResolutionRequest == null) {
      onLocationDisabled();
      return;
    }

    IntentSenderRequest intentSenderRequest = new IntentSenderRequest.Builder(pendingIntent.getIntentSender())
        .build();
    mResolutionRequest.launch(intentSenderRequest);
  }

  @Override
  @UiThread
  public void onLocationDisabled()
  {
    Logger.d(TAG, "provider = " + mLocationProvider + " permissions = " + LocationUtils.isLocationGranted(mContext) +
        " settings = " + LocationUtils.areLocationServicesTurnedOn(mContext) + " isAnnoying = " + mErrorDialogAnnoying);

    if (LocationUtils.areLocationServicesTurnedOn(mContext) &&
        !(mLocationProvider instanceof AndroidNativeProvider))
    {
      // If location service is enabled, try to downgrade to the native provider first
      // and restart the service before notifying the user.
      Logger.d(TAG, "Downgrading to use native provider");
      mLocationProvider = new AndroidNativeProvider(mContext, this);
      restart();
      return;
    }

    mSavedLocation = null;
    nativeOnLocationError(ERROR_GPS_OFF);

    if (mUiCallback == null || mErrorDialogAnnoying || (mErrorDialog != null && mErrorDialog.isShowing()))
      return;

    final AppCompatActivity activity = mUiCallback.requireActivity();
    AlertDialog.Builder builder = new AlertDialog.Builder(activity)
        .setTitle(R.string.enable_location_services)
        .setMessage(R.string.location_is_disabled_long_text)
        .setOnDismissListener(dialog -> mErrorDialog = null)
        .setOnCancelListener(dialog -> setLocationErrorDialogAnnoying(true))
        .setNegativeButton(R.string.close, (dialog, which) -> setLocationErrorDialogAnnoying(true));
    final Intent intent = Utils.makeSystemLocationSettingIntent(activity);
    if (intent != null)
    {
      intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
      intent.addFlags(Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
      builder.setPositiveButton(R.string.connection_settings, (dialog, which) -> activity.startActivity(intent));
    }
    mErrorDialog = builder.show();
  }

  @UiThread
  private void onLocationPendingTimeout()
  {
    Logger.d(TAG, "active = " + mActive + " permissions = " + LocationUtils.isLocationGranted(mContext) +
        " settings = " + LocationUtils.areLocationServicesTurnedOn(mContext) + " isAnnoying = " + mErrorDialogAnnoying);

    if (!mActive || !LocationUtils.isLocationGranted(mContext) || !LocationUtils.areLocationServicesTurnedOn(mContext))
      return;

    if (mUiCallback == null || mErrorDialogAnnoying || (mErrorDialog != null && mErrorDialog.isShowing()))
      return;

    final AppCompatActivity activity = mUiCallback.requireActivity();
    mErrorDialog = new AlertDialog.Builder(activity)
        .setTitle(R.string.current_location_unknown_title)
        .setMessage(R.string.current_location_unknown_message)
        .setOnDismissListener(dialog -> mErrorDialog = null)
        .setNegativeButton(R.string.current_location_unknown_stop_button, (dialog, which) ->
        {
          setStopLocationUpdateByUser(true);
          stop();
        })
        .setPositiveButton(R.string.current_location_unknown_continue_button, (dialog, which) ->
        {
          if (!isActive())
            start();
          switchToNextMode();
          setLocationErrorDialogAnnoying(true);
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
    Logger.d(TAG);
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
    Logger.d(TAG);
    if (LocationState.nativeGetMode() != LocationState.NOT_FOLLOW_NO_POSITION)
      start();
  }

  /**
   * Starts polling location updates.
   */
  public void start()
  {
    Logger.d(TAG);

    if (mActive)
    {
      Logger.w(TAG, "Provider '" + mLocationProvider + "' is already started");
      return;
    }

    if (mLocationUpdateStoppedByUser)
    {
      Logger.d(TAG, "Location updates are stopped by the user manually, so skip provider start"
               + " until the user starts it manually.");
      nativeOnLocationError(ERROR_GPS_OFF);
      return;
    }

    long oldInterval = mInterval;
    Logger.d(TAG, "Old time interval (ms): " + oldInterval);
    calcLocationUpdatesInterval();
    if (!LocationUtils.isLocationGranted(mContext))
    {
      Logger.w(TAG, "Dynamic permissions ACCESS_COARSE_LOCATION and/or ACCESS_FINE_LOCATION are not granted");
      mSavedLocation = null;
      nativeOnLocationError(ERROR_DENIED);
      if (mUiCallback != null)
        requestPermissions();
      return;
    }
    Logger.i(TAG, "start(): interval = " + mInterval + " provider = '" + mLocationProvider + "' mInFirstRun = " + mInFirstRun);
    checkForAgpsUpdates();
    mLocationProvider.start(mInterval);
    mSensorHelper.start();
    mActive = true;
  }

  /**
   * Stops the polling location updates.
   */
  public void stop()
  {
    Logger.d(TAG);

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
    Logger.d(TAG, "callback = " + callback);

    if (mUiCallback != null)
    {
      Logger.d(TAG, " - already attached. Skip.");
      return;
    }

    mUiCallback = callback;

    mPermissionRequest = mUiCallback.requireActivity()
        .registerForActivityResult(new ActivityResultContracts.RequestMultiplePermissions(),
            result -> onRequestPermissionsResult());
    mResolutionRequest = mUiCallback.requireActivity()
        .registerForActivityResult(new ActivityResultContracts.StartIntentSenderForResult(),
            result -> onLocationResolutionResult(result.getResultCode()));

    if (!Config.isScreenSleepEnabled()) {
      Utils.keepScreenOn(true, mUiCallback.requireActivity().getWindow());
    }

    LocationState.nativeSetLocationPendingTimeoutListener(this::onLocationPendingTimeout);
    LocationState.nativeSetListener(mUiCallback);
    mUiCallback.onMyPositionModeChanged(getMyPositionMode());
    if (mCompassData != null)
      mUiCallback.onCompassUpdated(mCompassData);

    if (mActive)
    {
      Logger.d(TAG, "attach() provider '" + mLocationProvider + "' is active, just add the listener");
      if (mSavedLocation != null)
        notifyLocationUpdated();
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
  public void detach()
  {
    Logger.d(TAG, "callback = " + mUiCallback);

    if (mUiCallback == null)
    {
      Logger.d(TAG, " - already detached. Skip.");
      return;
    }

    Utils.keepScreenOn(false, mUiCallback.requireActivity().getWindow());
    LocationState.nativeRemoveLocationPendingTimeoutListener();
    LocationState.nativeRemoveListener();
    mPermissionRequest.unregister();
    mPermissionRequest = null;
    mResolutionRequest.unregister();
    mResolutionRequest = null;
    mUiCallback = null;
  }

  @UiThread
  public void requestPermissions()
  {
    Logger.d(TAG);

    if (mUiCallback == null)
      throw new IllegalStateException("Not attached");

    mPermissionRequest.launch(new String[]{
        ACCESS_COARSE_LOCATION,
        ACCESS_FINE_LOCATION
    });
  }

  @UiThread
  private void onRequestPermissionsResult()
  {
    if (!LocationUtils.isLocationGranted(mContext))
    {
      Logger.w(TAG, "Permissions have not been granted");
      mSavedLocation = null;
      nativeOnLocationError(ERROR_DENIED);
      if (mUiCallback != null)
        mUiCallback.onLocationDenied();
      return;
    }

    Logger.i(TAG, "Permissions have been granted");
    setLocationErrorDialogAnnoying(false);
    setStopLocationUpdateByUser(false);
    restart();
  }

  @UiThread
  private void onLocationResolutionResult(int resultCode)
  {
    if (resultCode != Activity.RESULT_OK)
    {
      onLocationDisabled();
      return;
    }

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

  public interface UiCallback extends LocationState.ModeChangeListener
  {
    @NonNull
    AppCompatActivity requireActivity();
    void onLocationUpdated(@NonNull Location location);
    void onCompassUpdated(@NonNull CompassData compass);
    void onLocationDenied();
    void onRoutingFinish();
  }
}
