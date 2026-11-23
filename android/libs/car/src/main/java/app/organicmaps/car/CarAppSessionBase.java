package app.organicmaps.car;

import android.content.Intent;
import android.content.res.Configuration;
import android.location.Location;
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
import java.util.ArrayDeque;
import java.util.Objects;
import java.util.Queue;

public abstract class CarAppSessionBase
    extends Session implements DefaultLifecycleObserver, LocationState.ModeChangeListener, PlacePageActivationListener
{
  @NonNull
  private static final String TAG = CarAppSessionBase.class.getSimpleName();

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
  protected CarSensorsManager mSensorsManager;

  @NonNull
  private final LocationListener mSpeedLocationListener = this::updateSpeed;
  private boolean mSpeedLimitSignTypeInitialized;
  @NonNull
  private final MapsReadiness mMapsReadiness;
  private final Queue<Intent> mPendingIntents = new ArrayDeque<>();
  @Nullable
  private MapObject mPendingPlace;
  private boolean mRestoreRoutePending;
  private boolean mObservingViewport;
  @Nullable
  private String mPendingCountryId;
  @Nullable
  private String mLastOfferedCountryId;
  @NonNull
  private final Runnable mCarDisplayWhenReady = this::onCarDisplayReady;
  @NonNull
  private final Runnable mIntentWhenReady = this::processPendingIntents;
  @NonNull
  private final Runnable mPlaceWhenReady = this::showPendingPlace;
  private final DefaultLifecycleObserver mScreenClosedObserver = new DefaultLifecycleObserver() {
    @Override
    public void onDestroy(@NonNull LifecycleOwner owner)
    {
      // Screen result callbacks can finish or push another screen during removal.
      UiThread.runLater(CarAppSessionBase.this::resumeAfterScreenClosed);
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
    // A car surface can report its first viewport before the host starts the session.
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

    return prepareScreens();
  }

  @Override
  public final void onNewIntent(@NonNull Intent intent)
  {
    Logger.d(TAG, intent.toString());
    mPendingIntents.add(intent);
    if (isStarted())
      mMapsReadiness.runWhenReady(mIntentWhenReady);
  }

  private void processPendingIntents()
  {
    while (isStarted() && !mPendingIntents.isEmpty() && !deferUntilSetupCloses())
    {
      final Intent intent = mPendingIntents.remove();
      IntentUtils.processIntent(getCarContext(), mOrganicMapsContext, mSurfaceRenderer, mDisplayManager, intent);
    }
  }

  @CallSuper
  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    // A display handoff can call onStart() while the car host has stopped the Session.
    if (!isStarted())
      return;

    Logger.d(TAG);
    mSpeedLimitSignTypeInitialized = false;
    updateSpeed(mOrganicMapsContext.getLocationHelper().getSavedLocation());
    mOrganicMapsContext.getLocationHelper().addListener(mSpeedLocationListener);
    if (mDisplayManager.isCarDisplayUsed())
    {
      LocationState.nativeSetListener(this);
      Framework.nativePlacePageActivationListener(this);
    }

    if (LocationUtils.checkFineLocationPermission(getCarContext()))
      mSensorsManager.onStart();

    if (mDisplayManager.isCarDisplayUsed())
    {
      ThemeUtils.update(getCarContext(), mSurfaceRenderer.isRenderingActive());
      mRestoreRoutePending = true;
      mMapsReadiness.runWhenReady(mCarDisplayWhenReady);
    }
    if (!mPendingIntents.isEmpty())
      mMapsReadiness.runWhenReady(mIntentWhenReady);
  }

  @CallSuper
  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mRestoreRoutePending = false;
    mMapsReadiness.cancel(mCarDisplayWhenReady);
    mMapsReadiness.cancel(mPlaceWhenReady);
    // The host can still deliver queued intents after a display handoff.
    if (!isStarted())
      mMapsReadiness.cancel(mIntentWhenReady);
    mPendingPlace = null;

    mOrganicMapsContext.getLocationHelper().removeListener(mSpeedLocationListener);

    mSensorsManager.onStop();

    if (mDisplayManager.isCarDisplayUsed())
    {
      LocationState.nativeRemoveListener();
      Framework.nativeRemovePlacePageActivationListener(this);
    }
  }

  @CallSuper
  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    setViewportObservationEnabled(false);
  }

  protected final void setViewportObservationEnabled(boolean enabled)
  {
    if (enabled == mObservingViewport)
      return;
    if (enabled)
    {
      if (!mOrganicMapsContext.arePlatformAndCoreInitialized())
        return;
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
    if (!alreadyPending && canShowCarScreen() && mMapsReadiness.isReady())
      UiThread.runLater(this::showPendingCountry);
  }

  private void showPendingCountry()
  {
    if (mPendingCountryId == null || !canShowCarScreen() || mRestoreRoutePending)
      return;
    final Screen top = mScreenManager.getTop();
    if (top instanceof PlaceScreen || top instanceof NavigationScreen || RoutingController.get().isNavigating())
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

  private boolean deferUntilSetupCloses()
  {
    // Download progress or an error can sit above the required screen.
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

  private void resumeAfterScreenClosed()
  {
    if (!isStarted())
      return;
    if (mRestoreRoutePending)
      onCarDisplayReady();
    else
      schedulePendingCountry();
    if (mPendingPlace != null)
      showPendingPlace();
    if (!mPendingIntents.isEmpty())
      processPendingIntents();
  }

  private void onCarDisplayReady()
  {
    if (!canShowCarScreen() || deferUntilSetupCloses())
      return;
    if (DownloaderHelpers.isWorldMapsDownloadNeeded(mOrganicMapsContext.getFlavor()))
    {
      final Screen world = new DownloadMapsScreenBuilder(getCarContext(), mOrganicMapsContext)
                               .setDownloaderType(DownloadMapsScreenBuilder.DownloaderType.FirstLaunch)
                               .build();
      world.getLifecycle().addObserver(mScreenClosedObserver);
      mScreenManager.push(world);
      return;
    }
    mRestoreRoutePending = false;
    onRestoreRoute();
    schedulePendingCountry();
  }

  private void schedulePendingCountry()
  {
    // Let queued destinations and viewport reports settle before offering the region.
    if (mPendingCountryId != null)
      UiThread.runLater(this::showPendingCountry);
  }

  private boolean isStarted()
  {
    return getLifecycle().getCurrentState().isAtLeast(Lifecycle.State.STARTED);
  }

  private boolean canShowCarScreen()
  {
    // Display ownership can change before the handoff callback runs.
    return isStarted() && mDisplayManager.isCarDisplayUsed();
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
    final Screen screen = mScreenManager.getTop();
    if (screen instanceof BaseMapScreen)
      screen.invalidate();
  }

  @Override
  public final void onPlacePageActivated(@NonNull PlacePageData data)
  {
    if (!canShowCarScreen())
      return;
    mPendingPlace = (MapObject) data;
    mMapsReadiness.runWhenReady(mPlaceWhenReady);
  }

  private void showPendingPlace()
  {
    if (deferUntilSetupCloses())
      return;
    final MapObject mapObject = Objects.requireNonNull(mPendingPlace);
    mPendingPlace = null;
    if (!canShowCarScreen())
      return;
    // A place queued during startup can be delivered while a download prompt is visible.
    if (DownloadMapsScreen.MARKER.equals(mScreenManager.getTop().getMarker()))
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
    mScreenManager.popToRoot();
    mScreenManager.push(placeScreen);
  }

  @Override
  public final void onPlacePageDeactivated()
  {
    mPendingPlace = null;
    mMapsReadiness.cancel(mPlaceWhenReady);
    // The function is called when we close the PlaceScreen or when we enter the navigation mode.
    // We only need to handle the first case.
    if (!canShowCarScreen() || !(mScreenManager.getTop() instanceof PlaceScreen))
      return;

    // Do NOT cancel routing here. This callback fires synchronously from the C++ tap handler
    // (Framework::OnTapEvent → DeactivateMapSelection → m_onPlacePageClose), including the Ruler
    // "extend route by tap" path which wipes the Finish mark mid-handler and aborts when the
    // stack unwinds back to ContinueRouteToPoint. User-initiated close goes through
    // PlaceScreen.onBackPressed(), which already calls RoutingController.cancel() explicitly.
    mScreenManager.popToRoot();
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

    if (routingController.isPlanning() || isNavigating || routingController.hasSavedRoute())
    {
      final PlaceScreen placeScreen = new PlaceScreen.Builder(getCarContext(), mOrganicMapsContext, mSurfaceRenderer)
                                          .setMapObject(routingController.getEndPoint())
                                          .setDebugMode(mIsDebug)
                                          .build();
      mScreenManager.popToRoot();
      mScreenManager.push(placeScreen);
    }
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
