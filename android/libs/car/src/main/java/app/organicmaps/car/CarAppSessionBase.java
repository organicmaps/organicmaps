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
import app.organicmaps.car.screens.NavigationScreen;
import app.organicmaps.car.screens.PlaceScreen;
import app.organicmaps.car.screens.download.DownloadMapsScreen;
import app.organicmaps.car.util.CurrentCountryChangedListener;
import app.organicmaps.car.util.IntentUtils;
import app.organicmaps.car.util.ThemeUtils;
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
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.location.LocationState;
import app.organicmaps.sdk.location.LocationUtils;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.StringUtils;
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
  @NonNull
  protected final ScreenManager mScreenManager;
  @NonNull
  protected final CurrentCountryChangedListener mCurrentCountryChangedListener;
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
  @NonNull
  private final Runnable mCarDisplayWhenReady = this::onCarDisplayReady;
  @NonNull
  private final Runnable mIntentWhenReady = this::processPendingIntents;
  @NonNull
  private final Runnable mPlaceWhenReady = this::showPendingPlace;
  @NonNull
  private final RoutingController.NavigationStateListener mNavigationStateListener =
      active -> onNavigationStateChanged();

  public CarAppSessionBase(@NonNull OrganicMaps organicMapsContext, @NonNull DisplayManager displayManager,
                           @NonNull SessionInfo sessionInfo, boolean isDebug)
  {
    mOrganicMapsContext = organicMapsContext;
    mMapsReadiness = organicMapsContext.getMapsReadiness();
    mSessionInfo = sessionInfo;
    mDisplayManager = displayManager;
    mIsDebug = isDebug;
    mScreenManager = getCarContext().getCarService(ScreenManager.class);
    mCurrentCountryChangedListener = new CurrentCountryChangedListener();
    getLifecycle().addObserver(this);
  }

  @CallSuper
  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    Logger.d(TAG);
    mSurfaceRenderer =
        RendererFactory.create(getCarContext(), mDisplayManager, mOrganicMapsContext.getLocationHelper(), owner);
    mSensorsManager = new CarSensorsManager(getCarContext(), mOrganicMapsContext.getSensorHelper(),
                                            mOrganicMapsContext.getLocationHelper());
    RoutingController.get().addNavigationStateListener(mNavigationStateListener);
  }

  @CallSuper
  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    RoutingController.get().removeNavigationStateListener(mNavigationStateListener);
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
    // Preserve host delivery order, including intents received while stopped or maps are loading.
    while (isStarted() && !mPendingIntents.isEmpty())
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
    mMapsReadiness.cancel(mCarDisplayWhenReady);
    // A display handoff keeps the host Session started, so its queued intents still run once maps load.
    if (!isStarted())
      mMapsReadiness.cancel(mIntentWhenReady);
    mMapsReadiness.cancel(mPlaceWhenReady);
    mPendingPlace = null;

    mOrganicMapsContext.getLocationHelper().removeListener(mSpeedLocationListener);

    mSensorsManager.onStop();

    if (mDisplayManager.isCarDisplayUsed())
    {
      LocationState.nativeRemoveListener();
      Framework.nativeRemovePlacePageActivationListener(this);
    }

    mCurrentCountryChangedListener.onStop();
  }

  private void onCarDisplayReady()
  {
    if (!canShowCarScreen())
      return;

    // Downloaded regions look missing to the download prompt until maps are registered.
    mCurrentCountryChangedListener.onStart(getCarContext(), mOrganicMapsContext);
    onRestoreRoute();
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
    final MapObject mapObject = Objects.requireNonNull(mPendingPlace);
    mPendingPlace = null;
    if (!canShowCarScreen())
      return;
    // A place queued during startup can be delivered while a download prompt is visible.
    if (DownloadMapsScreen.MARKER.equals(mScreenManager.getTop().getMarker()))
      return;

    // Don't display the PlaceScreen for 'MY_POSITION', during navigation or while the phone follows a track
    // TODO (AndrewShkrob): Implement the 'Add stop' functionality
    final RoutingController routing = RoutingController.get();
    if (mapObject.isMyPosition() || routing.isNavigating() || routing.isFollowingTrack())
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

  // A phone track preview has no car screen attached to the routing controller, so entering navigation
  // and returning to the preview are presented from here. Ending the journey reaches NavigationScreen itself.
  private void onNavigationStateChanged()
  {
    final RoutingController routing = RoutingController.get();
    if (canShowCarScreen() && routing.isFollowingTrack() && (routing.isNavigating() || routing.isPlanning()))
      onRestoreRoute();
  }

  private void onRestoreRoute()
  {
    final RoutingController routingController = RoutingController.get();
    // The phone keeps track previews, the car shows only the navigation along a track.
    // Only a road-route stack is stale for a preview; permission, download and settings screens stay.
    if (routingController.isFollowingTrack())
    {
      if (routingController.isNavigating())
        showNavigationScreen();
      else if (hasRouteScreenInStack())
        mScreenManager.popToRoot();
      return;
    }

    final boolean isNavigating = routingController.isNavigating();
    final boolean hasNavigatingScreen = hasNavigationScreenInStack();

    if (!isNavigating && hasNavigatingScreen)
      mScreenManager.popToRoot();

    final boolean isCarNavigation = isNavigating && routingController.getLastRouterType() == PlaceScreen.ROUTER;
    if (isCarNavigation && hasNavigatingScreen)
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

  private void showNavigationScreen()
  {
    if (hasNavigationScreenInStack())
    {
      mScreenManager.popTo(NavigationScreen.MARKER);
      return;
    }
    mScreenManager.popToRoot();
    mScreenManager.push(new NavigationScreen.Builder(getCarContext(), mOrganicMapsContext, mSurfaceRenderer)
                            .setDebugMode(mIsDebug)
                            .build());
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

  private boolean hasRouteScreenInStack()
  {
    for (final Screen screen : mScreenManager.getScreenStack())
    {
      if (screen instanceof PlaceScreen || NavigationScreen.MARKER.equals(screen.getMarker()))
        return true;
    }
    return false;
  }
}
