package app.organicmaps.car;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.location.Location;
import android.util.Base64;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.Screen;
import androidx.car.app.ScreenManager;
import androidx.car.app.Session;
import androidx.car.app.SessionInfo;
import androidx.lifecycle.DefaultLifecycleObserver;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.api.MapRequest;
import app.organicmaps.car.screens.FreeDriveScreen;
import app.organicmaps.car.screens.MapScreen;
import app.organicmaps.car.screens.NavigationScreen;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreenBuilder;
import app.organicmaps.car.screens.download.DownloaderHelpers;
import app.organicmaps.car.util.IntentUtils;
import app.organicmaps.car.util.ThemeUtils;
import app.organicmaps.car.util.UserActionRequired;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.MapsReadiness;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.PlacePageActivationListener;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.car.CarSensorsManager;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.car.renderer.RendererFactory;
import app.organicmaps.sdk.car.screens.BaseMapScreen;
import app.organicmaps.sdk.display.DisplayManager;
import app.organicmaps.sdk.display.DisplayType;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.location.LocationState;
import app.organicmaps.sdk.location.LocationUtils;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.sdk.widget.placepage.PlacePageData;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayDeque;
import java.util.Objects;
import java.util.Queue;

public abstract class CarAppSessionBase
    extends Session implements DefaultLifecycleObserver, LocationState.ModeChangeListener, PlacePageActivationListener
{
  @NonNull
  private static final String TAG = CarAppSessionBase.class.getSimpleName();
  private static final String LAST_REQUEST = "LastAttemptDigest";

  @NonNull
  protected final OrganicMaps mOrganicMapsContext;
  @NonNull
  protected final SessionInfo mSessionInfo;
  @NonNull
  protected final DisplayManager mDisplayManager;
  protected final boolean mIsDebug;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  protected ScreenManager mScreenManager;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  protected Renderer mSurfaceRenderer;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private CarSensorsManager mSensorsManager;

  @NonNull
  private final LocationListener mSpeedLocationListener = this::updateSpeed;
  private boolean mSpeedLimitSignTypeInitialized;
  private boolean mCarDisplayActive;
  private boolean mRestoreRoutePending;
  private boolean mCheckWorldMaps;
  @NonNull
  private final MapsReadiness mMapsReadiness;
  private boolean mObservingViewport;
  @Nullable
  private String mPendingCountryId;
  @Nullable
  private String mLastOfferedCountryId;
  @NonNull
  private final Queue<Intent> mPendingIntents = new ArrayDeque<>();
  @Nullable
  private MapObject mPendingPlace;
  @NonNull
  private final Runnable mCarActionsWhenReady = this::processCarActions;
  @NonNull
  private final DefaultLifecycleObserver mScreenClosedObserver = new DefaultLifecycleObserver() {
    @Override
    public void onDestroy(@NonNull LifecycleOwner owner)
    {
      // Screen destruction can run inside pop/remove; finish that stack mutation before dispatching.
      UiThread.runLater(CarAppSessionBase.this::resumeCarActions);
    }
  };

  public CarAppSessionBase(@NonNull OrganicMaps organicMapsContext, @NonNull DisplayManager displayManager,
                           @NonNull SessionInfo sessionInfo, boolean isDebug)
  {
    mOrganicMapsContext = organicMapsContext;
    mMapsReadiness = organicMapsContext.getMapsReadiness();
    mSessionInfo = sessionInfo;
    mDisplayManager = displayManager;
    mIsDebug = isDebug;
    getLifecycle().addObserver(this);
  }

  @CallSuper
  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mScreenManager = getCarContext().getCarService(ScreenManager.class);
    setViewportObservationEnabled(mDisplayManager.isCarDisplayUsed());
    mSurfaceRenderer =
        RendererFactory.create(getCarContext(), mDisplayManager, mOrganicMapsContext.getLocationHelper(), owner);
    mSensorsManager = new CarSensorsManager(getCarContext(), mOrganicMapsContext.getSensorHelper(),
                                            mOrganicMapsContext.getLocationHelper());
  }

  @CallSuper
  @Override
  public void onCarConfigurationChanged(@NonNull Configuration newConfiguration)
  {
    Logger.d(TAG, "New configuration: " + newConfiguration);
    if (mDisplayManager.isCarDisplayUsed())
      ThemeUtils.update(getCarContext(), mSurfaceRenderer.isRenderingActive());
  }

  @NonNull
  @Override
  public final Screen onCreateScreen(@NonNull Intent intent)
  {
    Logger.d(TAG);

    Logger.i(TAG, "Session info: " + mSessionInfo);
    Logger.i(TAG, "API Level: " + getCarContext().getCarAppApiLevel());
    Logger.i(TAG, "Supported templates: " + mSessionInfo.getSupportedTemplates(getCarContext().getCarAppApiLevel()));
    Logger.i(TAG, "Host info: " + getCarContext().getHostInfo());
    Logger.i(TAG, "Car configuration: " + getCarContext().getResources().getConfiguration());

    // Build the stack for the current display owner before a request can switch it to the car.
    final Screen screen = prepareScreens();
    if (!isRepeatedRequest(intent))
      enqueueIntent(intent);
    return screen;
  }

  @Override
  public final void onNewIntent(@NonNull Intent intent)
  {
    Logger.d(TAG, intent.toString());
    // A genuine new receipt may repeat a destination while waiting for setup.
    if (isRepeatedRequest(intent))
      writeLastRequest(null);
    enqueueIntent(intent);
  }

  @CallSuper
  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    onCarDisplayAvailable();
  }

  @CallSuper
  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mMapsReadiness.cancel(mCarActionsWhenReady);
    deactivateCarDisplay();
  }

  @CallSuper
  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    setViewportObservationEnabled(false);
    // The same destination can start a different trip after a clean session end.
    requestHistory().edit().remove(LAST_REQUEST).apply();
  }

  protected final void setViewportObservationEnabled(boolean enabled)
  {
    if (enabled == mObservingViewport)
      return;
    if (enabled)
    {
      MapManager.nativeSubscribeOnCountryChanged(this::onCurrentCountryChanged);
    }
    else
    {
      MapManager.nativeUnsubscribeOnCountryChanged();
      mPendingCountryId = null;
    }
    mObservingViewport = enabled;
  }

  private void onCurrentCountryChanged(@NonNull String countryId)
  {
    if (!mDisplayManager.isCarDisplayUsed() || countryId.equals(mLastOfferedCountryId))
      return;
    mLastOfferedCountryId = null;
    if (countryId.isEmpty())
    {
      mPendingCountryId = null;
      return;
    }
    final boolean alreadyPending = mPendingCountryId != null;
    mPendingCountryId = countryId;
    // Installed regions look missing to CountryItem.fill() until registration completes.
    if (!alreadyPending && isCarDisplayReady() && mMapsReadiness.isReady())
      UiThread.runLater(this::showPendingCountry);
  }

  private void showPendingCountry()
  {
    if (mPendingCountryId == null || !isCarDisplayReady() || waitForRequiredSetup())
      return;
    final Screen top = mScreenManager.getTop();
    if (top instanceof NavigationScreen || RoutingController.get().isNavigating())
    {
      mPendingCountryId = null;
      return;
    }
    if (!(top instanceof MapScreen || top instanceof FreeDriveScreen))
    {
      top.getLifecycle().addObserver(mScreenClosedObserver);
      return;
    }
    final String countryId = mPendingCountryId;
    mPendingCountryId = null;
    if (CountryItem.fill(countryId).present)
      return;
    mLastOfferedCountryId = countryId;
    final Screen offer = new DownloadMapsScreenBuilder(getCarContext(), mOrganicMapsContext)
                             .setDownloaderType(DownloadMapsScreenBuilder.DownloaderType.View)
                             .setMissingMaps(new String[] {countryId})
                             .build();
    mScreenManager.pushForResult(offer, ignored -> {
      // Closing this offer also handles a return to the same region while it was open.
      if (countryId.equals(mPendingCountryId))
      {
        mLastOfferedCountryId = countryId;
        mPendingCountryId = null;
      }
    });
  }

  protected final void onCarDisplayAvailable()
  {
    activateCarDisplay();
    resumeCarActions();
  }

  private void activateCarDisplay()
  {
    if (mCarDisplayActive || !isStarted() || !mDisplayManager.isCarDisplayUsed()
        || !mSurfaceRenderer.isRenderingActive())
      return;

    mCarDisplayActive = true;
    mRestoreRoutePending = true;
    mCheckWorldMaps = true;
    mSpeedLimitSignTypeInitialized = false;
    updateSpeed(mOrganicMapsContext.getLocationHelper().getSavedLocation());
    mOrganicMapsContext.getLocationHelper().addListener(mSpeedLocationListener);
    LocationState.nativeSetListener(this);
    Framework.nativePlacePageActivationListener(this);
    startSensorsIfPermitted();
    ThemeUtils.update(getCarContext(), true);
  }

  protected final void deactivateCarDisplay()
  {
    if (!mCarDisplayActive)
      return;
    mCarDisplayActive = false;
    mRestoreRoutePending = false;
    mPendingPlace = null;
    mOrganicMapsContext.getLocationHelper().removeListener(mSpeedLocationListener);
    mSensorsManager.onStop();
    LocationState.nativeRemoveListener();
    Framework.nativeRemovePlacePageActivationListener(this);
  }

  protected final void startSensorsIfPermitted()
  {
    if (isCarDisplayReady() && LocationUtils.checkFineLocationPermission(getCarContext()))
      mSensorsManager.onStart();
  }

  private boolean isStarted()
  {
    return getLifecycle().getCurrentState().isAtLeast(Lifecycle.State.STARTED);
  }

  private boolean isCarDisplayReady()
  {
    // Display ownership can change before the handoff callback runs.
    return mCarDisplayActive && mDisplayManager.isCarDisplayUsed();
  }

  @NonNull
  protected abstract Screen prepareScreens();

  private void updateSpeed(@Nullable Location location)
  {
    if (location != null && !mSpeedLimitSignTypeInitialized)
    {
      mSurfaceRenderer.setSpeedLimitSignType(
          Framework.nativeGetSpeedLimitSignType(location.getLatitude(), location.getLongitude()));
      mSpeedLimitSignTypeInitialized = true;
    }
    mSurfaceRenderer.setCurrentSpeed(
        StringUtils.nativeFormatSpeed(location != null && location.hasSpeed() ? location.getSpeed() : 0),
        StringUtils.nativeGetLocalizedSpeedUnits());
  }

  @Override
  public final void onMyPositionModeChanged(int newMode)
  {
    if (!isCarDisplayReady())
      return;
    final Screen screen = mScreenManager.getTop();
    if (screen instanceof BaseMapScreen)
      screen.invalidate();
  }

  @Override
  public final void onPlacePageActivated(@NonNull PlacePageData data)
  {
    if (!isCarDisplayReady())
      return;
    mPendingPlace = (MapObject) data;
    if (mMapsReadiness.isReady())
      showPendingPlace();
  }

  private void showPendingPlace()
  {
    final boolean canShow = isCarDisplayReady();
    if (canShow && waitForRequiredSetup())
      return;
    final MapObject mapObject = Objects.requireNonNull(mPendingPlace);
    mPendingPlace = null;
    if (!canShow || DownloadMapsScreen.MARKER.equals(mScreenManager.getTop().getMarker()))
      return;
    // Don't display the PlaceScreen for 'MY_POSITION' or during navigation
    // TODO (AndrewShkrob): Implement the 'Add stop' functionality
    if (mapObject.isMyPosition() || RoutingController.get().isNavigating())
    {
      Framework.nativeDeactivatePopup();
      return;
    }
    final PlaceScreen placeScreen = new PlaceScreen.Builder(getCarContext(), mOrganicMapsContext, mSurfaceRenderer)
                                        .setDebugMode(mIsDebug)
                                        .setMapObject(mapObject)
                                        .build();
    // The picked place replaces a restored route that waits for an open screen to close.
    mRestoreRoutePending = false;
    mScreenManager.popToRoot();
    mScreenManager.push(placeScreen);
  }

  @Override
  public final void onPlacePageDeactivated()
  {
    mPendingPlace = null;
    // The function is called when we close the PlaceScreen or when we enter the navigation mode.
    // We only need to handle the first case.
    if (!isCarDisplayReady() || !(mScreenManager.getTop() instanceof PlaceScreen))
      return;

    // Do NOT cancel routing here. This callback fires synchronously from the C++ tap handler
    // (Framework::OnTapEvent → DeactivateMapSelection → m_onPlacePageClose), including the Ruler
    // "extend route by tap" path which wipes the Finish mark mid-handler and aborts when the
    // stack unwinds back to ContinueRouteToPoint. User-initiated close goes through
    // PlaceScreen.onBackPressed(), which already calls RoutingController.cancel() explicitly.
    mScreenManager.popToRoot();
  }

  private void resumeCarActions()
  {
    if (isStarted())
      mMapsReadiness.runWhenReady(mCarActionsWhenReady);
  }

  private void processCarActions()
  {
    if (waitForRequiredSetup())
      return;
    if (mPendingPlace != null)
      showPendingPlace();
    // A search during live guidance returns to the navigation screen when dismissed.
    if (mRestoreRoutePending && isCarDisplayReady() && RoutingController.get().isNavigating()
        && !hasNavigationScreenInStack())
    {
      mRestoreRoutePending = false;
      onRestoreRoute();
    }
    processPendingIntents();
    // A picked destination cancels the restore, and an opened search defers it.
    if (mRestoreRoutePending && isCarDisplayReady() && !waitForRouteWorkflow())
    {
      mRestoreRoutePending = false;
      onRestoreRoute();
    }
    if (mPendingCountryId != null)
      UiThread.runLater(this::showPendingCountry);
  }

  private boolean waitForRequiredSetup()
  {
    if (waitForRequiredScreen())
      return true;
    if (!mCheckWorldMaps || !isCarDisplayReady())
      return false;
    // Storage only knows which World maps are installed after map registration.
    if (DownloaderHelpers.isWorldMapsDownloadNeeded(mOrganicMapsContext.getFlavor()))
    {
      final Screen download = new DownloadMapsScreenBuilder(getCarContext(), mOrganicMapsContext)
                                  .setDownloaderType(DownloadMapsScreenBuilder.DownloaderType.FirstLaunch)
                                  .build();
      download.getLifecycle().addObserver(mScreenClosedObserver);
      mScreenManager.push(download);
      return true;
    }
    mCheckWorldMaps = false;
    return false;
  }

  private boolean waitForRequiredScreen()
  {
    // These actions can pop to root, including when another screen covers the required screen.
    for (Screen screen : mScreenManager.getScreenStack())
    {
      if (screen instanceof UserActionRequired)
      {
        screen.getLifecycle().addObserver(mScreenClosedObserver);
        return true;
      }
    }
    return false;
  }

  private boolean waitForRouteWorkflow()
  {
    if (mScreenManager.getStackSize() == 1 || (RoutingController.get().isNavigating() && !hasNavigationScreenInStack()))
      return false;
    final Screen top = mScreenManager.getTop();
    if (top instanceof PlaceScreen || top instanceof NavigationScreen)
      return false;
    // A restored plan must not replace an active search or settings workflow, so show it once that workflow closes.
    top.getLifecycle().addObserver(mScreenClosedObserver);
    return true;
  }

  private void processPendingIntents()
  {
    // A routing command can refer to a destination delivered by an earlier request.
    while (isStarted() && !mPendingIntents.isEmpty())
    {
      final Intent intent = mPendingIntents.element();
      if (IntentUtils.requiresCarDisplay(intent) && !isCarDisplayReady())
        return;
      mPendingIntents.remove();
      // Only the latest receipt can be replayed by the host after a process restart.
      if (mPendingIntents.isEmpty())
        writeLastRequest(requestDigest(intent));
      IntentUtils.processIntent(getCarContext(), mOrganicMapsContext, mSurfaceRenderer, intent);
    }
  }

  private SharedPreferences requestHistory()
  {
    return getCarContext().getSharedPreferences("CarRequests", Context.MODE_PRIVATE);
  }

  private boolean isRepeatedRequest(@NonNull Intent intent)
  {
    final String recorded = requestHistory().getString(LAST_REQUEST, null);
    // The host can repeat a request long after dispatch. An unclean disconnect can also leave the same
    // destination in the next trip indistinguishable from a replay.
    return recorded != null && recorded.equals(requestDigest(intent));
  }

  @SuppressLint("ApplySharedPref")
  private void writeLastRequest(@Nullable String digest)
  {
    final SharedPreferences prefs = requestHistory();
    if (Objects.equals(digest, prefs.getString(LAST_REQUEST, null)))
      return;
    // Finish the write before dispatch so a crashing request cannot keep restarting the process.
    if (!prefs.edit().putString(LAST_REQUEST, digest).commit())
      Logger.w(TAG, "Could not persist the car request state");
  }

  @Nullable
  private static String requestDigest(@NonNull Intent intent)
  {
    if (intent.getData() == null || intent.getAction() == null)
      return null;
    try
    {
      // Only the action and URI drive dispatch. Keep destination text out of preferences.
      final MessageDigest hash = MessageDigest.getInstance("SHA-256");
      hash.update(intent.getAction().getBytes(StandardCharsets.UTF_8));
      hash.update((byte) 0);
      return Base64.encodeToString(hash.digest(intent.getDataString().getBytes(StandardCharsets.UTF_8)),
                                   Base64.NO_WRAP);
    }
    catch (NoSuchAlgorithmException e)
    {
      throw new AssertionError(e);
    }
  }

  private void enqueueIntent(@NonNull Intent intent)
  {
    if (MapRequest.isExplicit(intent)
        && RoutingController.get().onExplicitMapRequest(MapRequest.addressesCurrentRoute(intent)))
      mRestoreRoutePending = true;
    mPendingIntents.add(intent);
    if (IntentUtils.requiresCarDisplay(intent) && !mDisplayManager.isCarDisplayUsed())
      mDisplayManager.changeDisplay(DisplayType.Car);
    resumeCarActions();
  }

  private void onRestoreRoute()
  {
    final RoutingController routingController = RoutingController.get();
    final boolean isNavigating = routingController.isNavigating();
    final boolean hasNavigatingScreen = hasNavigationScreenInStack();

    if (!isNavigating && hasNavigatingScreen)
      mScreenManager.popToRoot();

    if (isNavigating && routingController.getLastRouterType() == PlaceScreen.ROUTER && hasNavigatingScreen)
    {
      mScreenManager.popTo(NavigationScreen.MARKER);
      return;
    }

    final boolean liveRoute = routingController.isPlanning() || isNavigating;
    final MapObject destination = liveRoute ? routingController.getEndPoint() : null;
    // A phone plan can have only a start point. Leave the car map available until it has a destination.
    if (destination != null || (!liveRoute && routingController.hasSavedRoute()))
    {
      final PlaceScreen placeScreen = new PlaceScreen.Builder(getCarContext(), mOrganicMapsContext, mSurfaceRenderer)
                                          .setMapObject(destination)
                                          .setDebugMode(mIsDebug)
                                          .build();
      mScreenManager.popToRoot();
      mScreenManager.push(placeScreen);
    }
    else if (mScreenManager.getTop() instanceof PlaceScreen)
      mScreenManager.popToRoot();
  }

  private boolean hasNavigationScreenInStack()
  {
    for (final Screen screen : mScreenManager.getScreenStack())
    {
      if (NavigationScreen.MARKER.equals(screen.getMarker()))
        return true;
    }
    return false;
  }
}
