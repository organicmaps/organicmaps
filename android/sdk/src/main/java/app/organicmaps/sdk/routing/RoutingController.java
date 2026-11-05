package app.organicmaps.sdk.routing;

import android.text.TextUtils;
import androidx.annotation.IntRange;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.util.Pair;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import org.chromium.base.ObserverList;

/**
 * Main-thread, process-wide route planning and navigation coordinator.
 * Obtain it through {@link #get} after SDK initialization. Attach one {@link Container} for host UI callbacks,
 * call {@link #restore} when that host becomes active, and detach it when the host stops. The controller
 * retains route state across activity recreation; it does not provide the host foreground service or UI.
 * Detailed route building requires all relevant region maps. Missing-map errors are reported to the container.
 */
@androidx.annotation.UiThread
public class RoutingController
{
  private static final String TAG = RoutingController.class.getSimpleName();

  private enum State
  {
    NONE,
    PREPARE,
    NAVIGATION
  }

  /**
   * Current asynchronous build state; a selected profile or endpoints alone do not imply a usable route.
   */
  public enum BuildState
  {
    NONE,
    BUILDING,
    BUILT,
    ERROR
  }

  /** What a pending POI pick does with the picked place once it is committed. */
  enum PoiPickMode
  {
    /** Set the pick's own point type: Start or Finish. */
    SET,
    REPLACE,
    /** Continue the route: the picked place becomes the new destination. */
    APPEND
  }

  private record PendingPoiPick(@NonNull RouteMarkType pointType, @NonNull PoiPickMode mode, int replaceStopIndex)
  {
    private boolean isReplacement()
    {
      return mode == PoiPickMode.REPLACE;
    }
  }

  /**
   * UI callbacks for the single attached routing host. All callbacks run on the main thread.
   * Hosts that prepare routes must implement {@link #showRoutePlan} and invoke its completion callback.
   * Other default methods do nothing, allowing a host to implement only the controls it displays.
   */
  public interface Container
  {
    /**
     * Shows or hides planning UI. Invoke the supplied completion once that UI is ready, including when the host's
     * controls are always visible; {@link RoutingController#prepare} starts the build from this callback.
     *
     * @param show whether planning UI should be shown
     * @param completionListener optional continuation; invoke once after presenting the requested state
     */
    default void showRoutePlan(boolean show, @Nullable Runnable completionListener) {}
    default void showNavigation(boolean show) {}
    default void updateMenu() {}
    default void onNavigationCancelled() {}
    default void onNavigationStarted() {}
    default void onPlanningCancelled() {}
    default void onPlanningStarted() {}
    default void onAddedStop() {}
    default void onRemovedStop() {}
    default void onStopPointLimitReached() {}
    default void onPoiPickCompleted() {}
    default void onResetToPlanningState() {}
    default void onBuiltRoute() {}
    default void onDrivingOptionsWarning() {}

    default void onCommonBuildError(int lastResultCode, @NonNull String[] lastMissingMaps) {}
    default void onDrivingOptionsBuildError() {}

    /**
     * Reports build progress. A final progress update may arrive after a build result; read
     * {@link RoutingController#getBuildState} to decide whether the route is ready.
     *
     * @param progress completion percentage
     * @param router profile associated with the controller's current build
     */
    default void updateBuildProgress(@IntRange(from = 0, to = 100) int progress, Router router) {}
    default void onStartRouteBuilding() {}
  }

  /**
   * Process-scoped observer of navigation start/stop, independent of the UI {@link Container}. Fires
   * on every transition into or out of {@link State#NAVIGATION}, whatever the trigger -- phone UI,
   * Android Auto, notification stop, or route finished.
   */
  public interface NavigationStateListener
  {
    void onNavigationStateChanged(boolean active);
  }

  private static final RoutingController sInstance = new RoutingController();

  @Nullable
  private Container mContainer;

  @NonNull
  private final ObserverList<NavigationStateListener> mNavigationStateListeners = new ObserverList<>();

  private BuildState mBuildState = BuildState.NONE;
  private State mState = State.NONE;
  @Nullable
  private PendingPoiPick mPendingPoiPick;
  // Cached computeCanPickMyPosition() of the armed pick. Filled on the first ask, so arming a pick does not call into
  // the core; cleared with the pick in armPoiPick() and resetPoiPickState().
  @Nullable
  private Boolean mCanPickMyPosition;
  private int mLastBuildProgress;
  private Router mLastRouterType;
  private boolean mPendingOptionsRebuild;
  private boolean mHasContainerSavedState;
  private boolean mContainsCachedResult;
  private int mLastResultCode;
  private String[] mLastMissingMaps;
  @Nullable
  private RoutingInfo mCachedRoutingInfo;
  @Nullable
  private TransitRouteInfo mCachedTransitRouteInfo;

  private boolean mRouteSaved;

  @SuppressWarnings("FieldCanBeLocal")
  private final RoutingListener mRoutingListener = new RoutingListener() {
    @MainThread
    @Override
    public void onRoutingEvent(final int resultCode, @Nullable final String[] missingMaps)
    {
      Logger.d(TAG, "onRoutingEvent(resultCode: " + resultCode + ")");
      mLastResultCode = resultCode;
      mLastMissingMaps = missingMaps;
      mContainsCachedResult = true;

      if (mLastResultCode == ResultCodes.NO_ERROR || mLastResultCode == ResultCodes.HAS_WARNINGS)
        onBuiltRoute();

      processRoutingEvent();
    }
  };

  private void onBuiltRoute()
  {
    mCachedRoutingInfo = Framework.nativeGetRouteFollowingInfo();
    if (mLastRouterType == Router.Transit)
      mCachedTransitRouteInfo = Framework.nativeGetTransitRouteInfo();
    setBuildState(BuildState.BUILT);
    mLastBuildProgress = 100;
    if (mContainer != null)
      mContainer.onBuiltRoute();
  }

  private final RoutingProgressListener mRoutingProgressListener = progress ->
  {
    mLastBuildProgress = (int) progress;
    updateProgress();
  };

  private final RoutingLoadPointsListener mRoutingLoadPointsListener = success ->
  {
    if (success)
      prepare(getStartPoint(), getEndPoint());
  };

  public static RoutingController get()
  {
    return sInstance;
  }

  private void processRoutingEvent()
  {
    if (!mContainsCachedResult || mContainer == null || mHasContainerSavedState)
      return;

    mContainsCachedResult = false;

    if (mLastResultCode == ResultCodes.NO_ERROR || mLastResultCode == ResultCodes.HAS_WARNINGS)
    {
      if (mLastResultCode == ResultCodes.HAS_WARNINGS)
        mContainer.onDrivingOptionsWarning();
      updatePlan();
      return;
    }

    if (mLastResultCode == ResultCodes.CANCELLED)
    {
      setBuildState(BuildState.NONE);
      updatePlan();
      return;
    }

    // NEED_MORE_MAPS comes after a found route when the missing maps may give a better one, or instead of it.
    if (mLastResultCode != ResultCodes.NEED_MORE_MAPS || !isBuilt())
    {
      setBuildState(BuildState.ERROR);
      mLastBuildProgress = 0;
      updateProgress();
    }

    if (isDrivingOptionsBuildError())
      mContainer.onDrivingOptionsBuildError();
    else
      mContainer.onCommonBuildError(mLastResultCode, mLastMissingMaps);
  }

  private boolean isDrivingOptionsBuildError()
  {
    return mLastResultCode != ResultCodes.NEED_MORE_MAPS && RoutingOptions.hasAnyOptions(mLastRouterType);
  }

  private void setState(State newState)
  {
    Logger.d(TAG, "[S] State: " + mState + " -> " + newState + ", BuildState: " + mBuildState);
    final boolean wasNavigating = mState == State.NAVIGATION;
    mState = newState;
    final boolean isNavigating = mState == State.NAVIGATION;

    if (isNavigating != wasNavigating)
    {
      for (final NavigationStateListener listener : mNavigationStateListeners)
        listener.onNavigationStateChanged(isNavigating);
    }

    if (mContainer != null)
      mContainer.updateMenu();
  }

  /**
   * Registers a process-wide navigation observer, independent of the attached UI container.
   * Registration does not replay current state; read {@link #isNavigating} if an initial value is needed.
   */
  public void addNavigationStateListener(@NonNull NavigationStateListener listener)
  {
    mNavigationStateListeners.addObserver(listener);
  }

  public void removeNavigationStateListener(@NonNull NavigationStateListener listener)
  {
    mNavigationStateListeners.removeObserver(listener);
  }

  private void setBuildState(BuildState newState)
  {
    Logger.d(TAG, "[B] State: " + mState + ", BuildState: " + mBuildState + " -> " + newState);
    mBuildState = newState;

    if (mBuildState == BuildState.BUILT)
    {
      final MapObject startPoint = getStartPoint();
      if (startPoint == null || !startPoint.isMyPosition())
        Framework.nativeDisableFollowing();
    }

    if (mContainer != null)
      mContainer.updateMenu();
  }

  private void updateProgress()
  {
    if (mContainer != null)
      mContainer.updateBuildProgress(mLastBuildProgress, mLastRouterType);
  }

  private void showRoutePlan()
  {
    showRoutePlan(null, null);
  }

  private void showRoutePlan(final @Nullable MapObject startPoint, final @Nullable MapObject endPoint)
  {
    if (mContainer != null)
    {
      mContainer.showRoutePlan(true, () -> {
        if (startPoint == null || endPoint == null)
          updatePlan();
        else
          build();
      });
    }
  }

  /**
   * Replaces the single UI container. Call {@link #restore} to replay its current state after attachment.
   */
  public void attach(@NonNull Container container)
  {
    mContainer = container;
  }

  /**
   * Installs native routing callbacks using the application location helper. Called automatically by SDK
   * initialization.
   */
  public void initialize(@NonNull LocationHelper locationHelper)
  {
    mLastRouterType = Router.getLastUsed();

    Framework.nativeSetRoutingListener(mRoutingListener);
    Framework.nativeSetRouteProgressListener(mRoutingProgressListener);
    Framework.nativeSetRoutingRecommendationListener(recommendation -> UiThread.run(() -> {
      if (recommendation == RouteRecommendationType.RebuildAfterPointsLoading)
        setStartPoint(locationHelper.getMyPosition());
    }));
    Framework.nativeSetRoutingLoadPointsListener(mRoutingLoadPointsListener);
  }

  /**
   * Releases the attached UI container without cancelling the route or stopping navigation.
   */
  public void detach()
  {
    mContainer = null;
  }

  /**
   * Replays planning, navigation, build progress, and any deferred result to the attached container.
   * Call after attaching a resumed host; this also clears the saved-UI-state flag.
   */
  @MainThread
  public void restore()
  {
    mHasContainerSavedState = false;
    if (isPlanning())
      showRoutePlan();

    if (mContainer != null)
    {
      mContainer.showNavigation(isNavigating());
      mContainer.updateMenu();
      updateProgress();
    }
    processRoutingEvent();
  }

  /**
   * Defers UI result processing while the host has saved its state; {@link #restore} resumes processing.
   */
  public void onSaveState()
  {
    mHasContainerSavedState = true;
  }

  private void build()
  {
    Framework.nativeRemoveRoute();

    Logger.d(TAG, "build");
    mLastBuildProgress = 0;
    mRouteSaved = false;

    setBuildState(BuildState.BUILDING);
    if (mContainer != null)
      mContainer.onStartRouteBuilding();

    updatePlan();

    // This build uses the current settings and route marks, so it consumes any deferred options change.
    mPendingOptionsRebuild = false;
    Framework.nativeBuildRoute();
  }

  /**
   * Requests loading of route points previously saved by the native framework.
   */
  public void restoreRoute()
  {
    Framework.nativeLoadRoutePoints();
  }

  public boolean hasSavedRoute()
  {
    return Framework.nativeHasSavedRoutePoints();
  }

  /**
   * Persists usable planned or navigating route points for process-restart restoration.
   */
  public void saveRoute()
  {
    if (isNavigating() || (isPlanning() && isBuilt()))
      Framework.nativeSaveRoutePoints();
    else if (isPlanning())
      // A restart must not bring back an earlier route of this planning.
      Framework.nativeDeleteSavedRoutePoints();
  }

  /**
   * Deletes persisted route points without cancelling the current in-memory route.
   */
  public void deleteSavedRoute()
  {
    Framework.nativeDeleteSavedRoutePoints();
  }

  public boolean isRouteSaved()
  {
    return mRouteSaved;
  }

  /**
   * Marks the current route as saved by the host; this does not persist route points.
   */
  public void setRouteSaved()
  {
    mRouteSaved = true;
  }

  /**
   * Tries to reduce the straight-line distance between route marks by reordering intermediate stops. The core leaves
   * the marks unchanged while navigating, in Ruler mode, without both endpoints, or with fewer than three points.
   *
   * @return whether the order changed, which means the route has to be rebuilt.
   */
  public boolean optimizeRoutePoints()
  {
    return Framework.nativeOptimizeRoutePoints();
  }

  /**
   * Requests a rebuild after a change on the routing options screen. A planned route is rebuilt immediately when a
   * container is attached, including Android Auto. Otherwise the request waits for {@link #applyPendingRoutingOptions}
   * when the map activity starts. Leaving the options screen with Back normally reattaches the map first.
   */
  public void onRoutingOptionsChanged()
  {
    mPendingOptionsRebuild = true;
    applyPendingRoutingOptions();
  }

  /**
   * Applies a change recorded by {@link #onRoutingOptionsChanged}, if any. Called from there and again by the map
   * activity when it starts; {@link #attach} does not call it, because Android Auto screens attach to the same
   * controller too and would waste a build.
   */
  public void applyPendingRoutingOptions()
  {
    if (!mPendingOptionsRebuild || mContainer == null)
      return;

    mPendingOptionsRebuild = false;
    // A rebuild would exit navigation, so nothing is applied while navigating: the optimization switch then
    // affects only the stops added afterwards, and changed road types only the next route.
    if (isPlanning())
      rebuildLastRoute();
  }

  /**
   * Returns to planning and rebuilds the current endpoints with the selected profile and settings.
   */
  public void rebuildLastRoute()
  {
    setState(State.NONE);
    setBuildState(BuildState.NONE);
    prepare(getStartPoint(), getEndPoint());
  }

  /**
   * Starts a new route plan and selects a suggested profile for the endpoints.
   * @param startPoint origin, or {@code null} to leave it unresolved
   * @param endPoint destination, or {@code null} to leave it unresolved
   */
  public void prepare(@Nullable MapObject startPoint, @Nullable MapObject endPoint)
  {
    Logger.d(TAG, "prepare (" + (endPoint == null ? "route)" : "p2p)"));
    initLastRouteType(startPoint, endPoint);
    prepare(startPoint, endPoint, mLastRouterType);
  }

  private void initLastRouteType(@Nullable MapObject startPoint, @Nullable MapObject endPoint)
  {
    if (startPoint != null && endPoint != null)
      mLastRouterType = Router.getBest(startPoint.getLat(), startPoint.getLon(), endPoint.getLat(), endPoint.getLon());
  }

  /**
   * Replaces the current route plan and starts an asynchronous build when both endpoints are available.
   * @param startPoint origin, or {@code null} to leave it unresolved
   * @param endPoint destination, or {@code null} to leave it unresolved
   * @param routerType requested route profile
   */
  public void prepare(final @Nullable MapObject startPoint, final @Nullable MapObject endPoint, Router routerType)
  {
    cancel();
    setState(State.PREPARE);

    mLastRouterType = routerType;
    Router.set(mLastRouterType);

    if (startPoint != null || endPoint != null)
      setPointsInternal(startPoint, endPoint);

    startPlanning(startPoint, endPoint);
  }

  /**
   * Starts following a successfully built planned route; otherwise does nothing.
   * The host supplies location updates and any foreground-service lifecycle required for navigation.
   */
  public void start()
  {
    // Phone and car actions can outlive the built route they were created for.
    if (!isPlanning() || !isBuilt())
      return;
    Logger.d(TAG, "start");

    // This saving is needed just for situation when the user starts navigation
    // and then app crashes. So, the previous route will be restored on the next app launch.
    saveRoute();

    // Clear stale elevation preview marker.
    Framework.nativeRouteRemoveElevationActivePoint();
    setState(State.NAVIGATION);

    cancelPlanning(false);
    startNavigation();

    Framework.nativeFollowRoute();
  }

  /**
   * Commits a place to a previously armed point replacement, then rebuilds the route.
   * @throws IllegalStateException if no replacement was requested
   */
  public void replaceStop(@NonNull MapObject mapObject)
  {
    final PendingPoiPick pick = requirePendingPoiPick();
    if (!pick.isReplacement())
      throw new IllegalStateException("A route point replacement was not requested");
    replaceRoutePoint(pick.pointType(), mapObject, pick.replaceStopIndex());
    finalizeStopChange();
  }

  /**
   * Adds an intermediate stop and rebuilds. Reports the native point limit through the container if rejected.
   */
  public void addStop(@NonNull MapObject mapObject)
  {
    if (!addRoutePoint(RouteMarkType.Intermediate, mapObject))
    {
      if (mContainer != null)
        mContainer.onStopPointLimitReached();
      // Rejection completes the pending pick too, so its overlay is dismissed with the controls it owns.
      finalizePendingPoiPick();
      return;
    }

    finalizeStopChange();
  }

  private void appendStop(@NonNull MapObject mapObject)
  {
    if (!appendRoutePoint(mapObject))
    {
      if (mContainer != null)
        mContainer.onStopPointLimitReached();
      finalizePendingPoiPick();
      return;
    }

    finalizeStopChange();
  }

  /**
   * Commits a selected place according to the currently armed add, replace, or append operation.
   * With no pick armed, the mode is SET and a plain place-page selection inserts an intermediate point.
   */
  public void commitStopPick(@NonNull MapObject mapObject)
  {
    switch (getPoiPickMode())
    {
    case REPLACE -> replaceStop(mapObject);
    case APPEND -> appendStop(mapObject);
    case SET -> addStop(mapObject);
    }
  }

  private void finalizeStopChange()
  {
    build();
    if (mContainer != null)
      mContainer.onAddedStop();
    if (isNavigating())
      transitionToPlanning();
    resetPoiPickState();
  }

  /**
   * Removes a point carrying route-point metadata, then rebuilds the remaining route.
   */
  public void removeStop(@NonNull MapObject mapObject)
  {
    RoutePointInfo info = mapObject.getRoutePointInfo();
    if (info == null)
      throw new AssertionError("A stop point must have the route point info!");

    removeStop(info.mMarkType, info.mIntermediateIndex);
  }

  /**
   * Removes the specified point; removing the only point cancels planning.
   * @param type native point role
   * @param intermediateIndex zero-based intermediate index; ignored for endpoint roles
   */
  public void removeStop(@NonNull RouteMarkType type, int intermediateIndex)
  {
    if (Framework.nativeGetRoutePoints().length == 1)
    {
      cancel();
      return;
    }

    Framework.nativeRemoveRoutePoint(type, intermediateIndex);
    build();
    if (mContainer != null)
      mContainer.onRemovedStop();
    if (isNavigating())
      transitionToPlanning();
    resetPoiPickState();
  }

  /**
   * Rebuilds the existing route points and opens planning in the attached container.
   */
  public void launchPlanning()
  {
    build();
    setState(State.PREPARE);
    startPlanning();
    if (mContainer != null)
      mContainer.updateMenu();
    if (mContainer != null)
      mContainer.onResetToPlanningState();
  }

  /**
   * Returns navigation to planning and rebuilds.
   * @return whether navigation was active and a transition was requested
   */
  public boolean resetToPlanningStateIfNavigating()
  {
    if (isNavigating())
    {
      build();
      transitionToPlanning();
      return true;
    }
    return false;
  }

  // Each caller has already requested a route build; this only changes the navigation and planning UI.
  private void transitionToPlanning()
  {
    setState(State.PREPARE);
    cancelNavigation(false);
    startPlanning();
    if (mContainer != null)
      mContainer.updateMenu();
    if (mContainer != null)
      mContainer.onResetToPlanningState();
  }

  @NonNull
  private MapObject toMapObject(@NonNull RouteMarkData point)
  {
    return MapObject.createMapObject(point.mIsMyPosition ? MapObject.MY_POSITION : MapObject.POI,
                                     point.mTitle == null ? "" : point.mTitle,
                                     point.mSubtitle == null ? "" : point.mSubtitle, point.mLat, point.mLon);
  }

  /**
   * Returns whether a further intermediate stop is allowed in the current profile and controller state.
   */
  public boolean isStopPointAllowed()
  {
    return Framework.nativeCouldAddIntermediatePoint();
  }

  public boolean isPoiPickReplaceStop()
  {
    return mPendingPoiPick != null && mPendingPoiPick.isReplacement();
  }

  @NonNull
  PoiPickMode getPoiPickMode()
  {
    return mPendingPoiPick == null ? PoiPickMode.SET : mPendingPoiPick.mode();
  }

  public boolean isRoutePoint(@NonNull MapObject mapObject)
  {
    return mapObject.getRoutePointInfo() != null;
  }

  private void updatePlan()
  {
    updateProgress();
  }

  private void cancelInternal(boolean deleteSavedRoute)
  {
    Logger.d(TAG, "cancelInternal");

    resetPoiPickState();

    setBuildState(BuildState.NONE);
    setState(State.NONE);

    // Clear stale elevation preview marker.
    Framework.nativeRouteRemoveElevationActivePoint();
    if (deleteSavedRoute)
      Framework.nativeDeleteSavedRoutePoints();
    Framework.nativeCloseRouting();
  }

  /**
   * Cancels planning or navigation and deletes persisted route points.
   * @return whether a planning or navigation session was cancelled
   */
  public boolean cancel()
  {
    return cancel(true);
  }

  /**
   * Cancels planning or navigation and clears the route presentation.
   * @param deleteSavedRoute whether to delete persisted route points too
   * @return whether a planning or navigation session was cancelled
   */
  public boolean cancel(boolean deleteSavedRoute)
  {
    if (isPlanning())
    {
      Logger.d(TAG, "cancel: planning");

      cancelInternal(deleteSavedRoute);
      cancelPlanning(true);
      return true;
    }

    if (isNavigating())
    {
      Logger.d(TAG, "cancel: navigating");

      cancelInternal(deleteSavedRoute);
      cancelNavigation(true);
      if (mContainer != null)
      {
        mContainer.updateMenu();
      }
      return true;
    }

    Logger.d(TAG, "cancel: none");
    return false;
  }

  private void startPlanning()
  {
    if (mContainer != null)
    {
      showRoutePlan();
    }
  }

  private void startPlanning(final @Nullable MapObject startPoint, final @Nullable MapObject endPoint)
  {
    if (mContainer != null)
    {
      showRoutePlan(startPoint, endPoint);
      mContainer.onPlanningStarted();
    }
  }

  private void cancelPlanning(boolean fireEvent)
  {
    if (mContainer != null)
    {
      mContainer.showRoutePlan(false, null);
      if (fireEvent)
        mContainer.onPlanningCancelled();
    }
  }

  private void startNavigation()
  {
    if (mContainer != null)
    {
      mContainer.showNavigation(true);
      mContainer.onNavigationStarted();
    }
  }

  private void cancelNavigation(boolean fireEvent)
  {
    if (mContainer != null)
    {
      mContainer.showNavigation(false);
      if (fireEvent)
        mContainer.onNavigationCancelled();
    }
  }

  public boolean isPlanning()
  {
    return mState == State.PREPARE;
  }

  public boolean isTransitType()
  {
    return mLastRouterType == Router.Transit;
  }

  /**
   * Returns whether the selected profile is driving, independently of navigation state.
   */
  public boolean isVehicleRouterType()
  {
    return mLastRouterType == Router.Vehicle;
  }

  public boolean isRulerRouterType()
  {
    return mLastRouterType == Router.Ruler;
  }

  public boolean isNavigating()
  {
    return mState == State.NAVIGATION;
  }

  public boolean isVehicleNavigation()
  {
    return isNavigating() && isVehicleRouterType();
  }

  public boolean isBuilding()
  {
    return mState == State.PREPARE && mBuildState == BuildState.BUILDING;
  }

  public boolean isErrorEncountered()
  {
    return mBuildState == BuildState.ERROR;
  }

  public boolean isBuilt()
  {
    return mBuildState == BuildState.BUILT;
  }

  /**
   * Arms a map pick for a start or finish point role.
   */
  public void waitForPoiPick(@NonNull RouteMarkType pointType)
  {
    if (pointType == RouteMarkType.Intermediate)
      throw new IllegalArgumentException("A stop is picked with waitForPoiPickToAppend()");
    armPoiPick(pointType, PoiPickMode.SET, -1);
  }

  /**
   * Arms replacement of an existing point; intermediate indexes are zero-based.
   */
  public void waitForPoiReplacement(@NonNull RouteMarkType pointType, int index)
  {
    armPoiPick(pointType, PoiPickMode.REPLACE, index);
  }

  /**
   * Arms appending a new destination; the previous destination becomes an intermediate stop.
   */
  public void waitForPoiPickToAppend()
  {
    // Intermediate selects the "Add destination" labels and keeps the existing finish from being treated
    // as replaced when checking whether My Position can be picked. The core makes the picked point Finish.
    armPoiPick(RouteMarkType.Intermediate, PoiPickMode.APPEND, -1);
  }

  private void armPoiPick(@NonNull RouteMarkType pointType, @NonNull PoiPickMode mode, int replaceStopIndex)
  {
    mPendingPoiPick = new PendingPoiPick(pointType, mode, replaceStopIndex);
    mCanPickMyPosition = null;
  }

  /**
   * Returns whether the host is waiting for a route-point replacement or append pick.
   * A stop pick commits through the place page's Add/Replace button; an empty endpoint slot uses the regular
   * routing buttons.
   */
  public boolean isWaitingStopPick()
  {
    return getPoiPickMode() != PoiPickMode.SET;
  }

  /**
   * Cancels the armed point pick without changing the route points.
   * A selection is committed before closing search or the map chooser; this drops state for an applied or abandoned
   * pick.
   */
  public void cancelPoiPick()
  {
    resetPoiPickState();
  }

  private void finalizePendingPoiPick()
  {
    if (!isWaitingPoiPick())
      return;
    resetPoiPickState();
    if (mContainer != null)
      mContainer.onPoiPickCompleted();
  }

  private void resetPoiPickState()
  {
    mPendingPoiPick = null;
    mCanPickMyPosition = null;
  }

  public boolean isWaitingPoiPick()
  {
    return mPendingPoiPick != null;
  }

  @NonNull
  private PendingPoiPick requirePendingPoiPick()
  {
    if (mPendingPoiPick == null)
      throw new IllegalStateException("A route point pick was not requested");
    return mPendingPoiPick;
  }

  // SET and REPLACE overwrite their target slot, so a my-position point there is replaced rather than
  // duplicated. APPEND preserves the current finish as a stop and therefore replaces no existing point.
  private static boolean isReplacedByPick(@NonNull RouteMarkData point, @NonNull PendingPoiPick pick)
  {
    if (point.mPointType != pick.pointType())
      return false;
    if (pick.pointType() != RouteMarkType.Intermediate)
      return true;
    return pick.isReplacement() && point.mIntermediateIndex == pick.replaceStopIndex();
  }

  // The core keeps a single my-position mark, so adding it to a second slot pulls it out of the one it
  // already holds (in AddRoutePoint and ContinueRouteToPoint), silently emptying that one. Offer the shortcut only
  // where the route has no such point, or where the pick replaces the one it has. Cache this per pick
  // because the search UI asks on every location update; arming or clearing the pick invalidates the cache.
  private static boolean computeCanPickMyPosition(@NonNull PendingPoiPick pick)
  {
    for (RouteMarkData point : Framework.nativeGetRoutePoints())
      if (point.mIsMyPosition && !isReplacedByPick(point, pick))
        return false;
    return true;
  }

  /**
   * Returns whether current location can fill the armed point pick.
   */
  public boolean canPickMyPosition()
  {
    if (mPendingPoiPick == null)
      return false;
    if (mCanPickMyPosition == null)
      mCanPickMyPosition = computeCanPickMyPosition(mPendingPoiPick);
    return mCanPickMyPosition;
  }

  public BuildState getBuildState()
  {
    return mBuildState;
  }

  @Nullable
  public MapObject getStartPoint()
  {
    return getStartOrEndPointByType(RouteMarkType.Start);
  }

  @Nullable
  public MapObject getEndPoint()
  {
    return getStartOrEndPointByType(RouteMarkType.Finish);
  }

  @Nullable
  private MapObject getStartOrEndPointByType(@NonNull RouteMarkType type)
  {
    RouteMarkData[] points = Framework.nativeGetRoutePoints();
    int size = points.length;

    if (size == 0)
      return null;

    if (size == 1)
    {
      RouteMarkData point = points[0];
      return point.mPointType == type ? toMapObject(point) : null;
    }

    if (type == RouteMarkType.Start)
      return toMapObject(points[0]);
    if (type == RouteMarkType.Finish)
      return toMapObject(points[size - 1]);

    return null;
  }

  @Nullable
  public RoutingInfo getCachedRoutingInfo()
  {
    return mCachedRoutingInfo;
  }

  @Nullable
  public TransitRouteInfo getCachedTransitInfo()
  {
    return mCachedTransitRouteInfo;
  }

  private void setPointsInternal(@Nullable MapObject startPoint, @Nullable MapObject endPoint)
  {
    final boolean hasStart = startPoint != null;
    final boolean hasEnd = endPoint != null;
    final boolean hasOnePointAtLeast = hasStart || hasEnd;

    // The result is unread on purpose: the core drops the point standing in the slot before adding, so only adding
    // a stop can run the route out of capacity.
    if (hasStart)
      addRoutePoint(RouteMarkType.Start, startPoint);

    if (hasEnd)
      addRoutePoint(RouteMarkType.Finish, endPoint);

    if (hasOnePointAtLeast && mContainer != null)
      mContainer.updateMenu();
  }

  /**
   * Builds the plan when both endpoints are available.
   */
  public void checkAndBuildRoute()
  {
    // showRoutePlan() runs while POI-pick is still pending on purpose: the plan sheet lands underneath the
    // search/PP overlay that finalizePendingPoiPick() dismisses right after (via set{Start,End}Point's outer
    // wrapper). Flipping this order would visibly pop the sheet up post-close instead of revealing it seamlessly.
    if (isWaitingPoiPick())
      showRoutePlan();

    if (getStartPoint() != null && getEndPoint() != null)
      build();
  }

  /**
   * Sets starting point.
   * <ul>
   *   <li>If {@code point} matches ending one and the starting point was set &mdash; swap points.
   *   <li>The same as the currently set starting point is skipped.
   * </ul>
   * Route starts to build if both points were set.
   *
   * @return {@code true} if the point was set.
   */
  @SuppressWarnings("Duplicates")
  public boolean setStartPoint(@Nullable MapObject point)
  {
    final boolean result = setStartPointInternal(point);
    finalizePendingPoiPick();
    return result;
  }

  private boolean setStartPointInternal(@Nullable MapObject point)
  {
    Logger.d(TAG, "setStartPoint");
    MapObject startPoint = getStartPoint();
    MapObject endPoint = getEndPoint();
    boolean isSamePoint = MapObject.same(startPoint, point);
    if (isSamePoint)
    {
      Logger.d(TAG, "setStartPoint: skip the same starting point");
      return false;
    }

    if (point != null && point.sameAs(endPoint))
    {
      if (startPoint == null)
      {
        Logger.d(TAG, "setStartPoint: skip because starting point is empty");
        return false;
      }

      Logger.d(TAG, "setStartPoint: swap with end point");
      endPoint = startPoint;
    }

    startPoint = point;
    setPointsInternal(startPoint, endPoint);
    checkAndBuildRoute();
    return true;
  }

  /**
   * Sets ending point.
   * <ul>
   *   <li>If {@code point} is the same as starting point &mdash; swap points if ending point is set, skip otherwise.
   * </ul>
   * Route starts to build if both points were set.
   *
   * @return {@code true} if the point was set.
   */
  @SuppressWarnings("Duplicates")
  public boolean setEndPoint(@Nullable MapObject point)
  {
    final boolean result = setEndPointInternal(point);
    finalizePendingPoiPick();
    return result;
  }

  private boolean setEndPointInternal(@Nullable MapObject point)
  {
    Logger.d(TAG, "setEndPoint");
    MapObject startPoint = getStartPoint();
    MapObject endPoint = getEndPoint();
    boolean isSamePoint = MapObject.same(endPoint, point);
    if (isSamePoint)
      return false;

    if (point != null && point.sameAs(startPoint))
    {
      if (endPoint == null)
        return false;

      startPoint = endPoint;
    }

    endPoint = point;
    setPointsInternal(startPoint, endPoint);
    checkAndBuildRoute();
    return true;
  }

  private static void replaceRoutePoint(@NonNull RouteMarkType type, @NonNull MapObject point, int replaceStopIndex)
  {
    Pair<String, String> description = getDescriptionForPoint(point);
    Framework.nativeReplaceRoutePoint(description.first /* title */, description.second /* subtitle */, type,
                                      replaceStopIndex /* intermediateIndex */, point.isMyPosition(), point.getLat(),
                                      point.getLon());
  }

  private static boolean addRoutePoint(@NonNull RouteMarkType type, @NonNull MapObject point)
  {
    Pair<String, String> description = getDescriptionForPoint(point);
    return Framework.nativeAddRoutePoint(description.first /* title */, description.second /* subtitle */, type,
                                         point.isMyPosition(), point.getLat(), point.getLon(),
                                         true /* allowOptimization */);
  }

  private static boolean appendRoutePoint(@NonNull MapObject point)
  {
    Pair<String, String> description = getDescriptionForPoint(point);
    return Framework.nativeContinueRouteToPoint(description.first /* title */, description.second /* subtitle */,
                                                point.isMyPosition(), point.getLat(), point.getLon());
  }

  @NonNull
  private static Pair<String, String> getDescriptionForPoint(@NonNull MapObject point)
  {
    String title, subtitle = "";
    if (!TextUtils.isEmpty(point.getTitle()))
    {
      title = point.getTitle();
      subtitle = point.getSubtitle();
    }
    else
    {
      if (!TextUtils.isEmpty(point.getSubtitle()))
      {
        title = point.getSubtitle();
      }
      else if (!TextUtils.isEmpty(point.getAddress()))
      {
        title = point.getAddress();
      }
      else
      {
        title = Framework.nativeFormatLatLon(point.getLat(), point.getLon(), Framework.COORDINATES_FORMAT_DECIMAL);
      }
    }
    return new Pair<>(title, subtitle);
  }

  /**
   * Exchanges the endpoint roles and rebuilds the plan.
   */
  public void swapPoints()
  {
    Logger.d(TAG, "swapPoints");

    MapObject startPoint = getStartPoint();
    MapObject endPoint = getEndPoint();
    MapObject point = startPoint;
    startPoint = endPoint;
    endPoint = point;

    setPointsInternal(startPoint, endPoint);
    checkAndBuildRoute();
    if (mContainer != null)
      mContainer.updateMenu();
  }

  /**
   * Reverses the full route-point order, including intermediate stops, and rebuilds.
   */
  public void reverseRoute()
  {
    Logger.d(TAG, "reverseRoute");

    if (Framework.nativeReverseRoutePoints())
      build();
  }

  /**
   * Changes the routing profile and rebuilds when the profile differs.
   */
  public void setRouterType(Router router)
  {
    Logger.d(TAG, "setRouterType: " + mLastRouterType + " -> " + router);

    // Nothing to rebuild when the already selected router type is tapped again.
    if (router == mLastRouterType)
      return;

    mLastRouterType = router;
    Router.set(router);

    if (getStartPoint() != null && getEndPoint() != null)
      build();
  }

  public Router getLastRouterType()
  {
    return mLastRouterType;
  }

  /**
   * Commits a selected place to the armed endpoint or stop-pick operation.
   */
  public void onPoiSelected(@NonNull MapObject point)
  {
    final PendingPoiPick pick = mPendingPoiPick;
    if (pick == null)
      return;

    if (isWaitingStopPick())
      commitStopPick(point);
    else if (pick.pointType() == RouteMarkType.Finish)
      setEndPoint(point);
    else
      setStartPoint(point);

    if (mContainer != null)
    {
      mContainer.updateMenu();
      showRoutePlan();
    }

    resetPoiPickState();
  }

  /**
   * Returns the point role of the armed pick, or {@code null} when none is armed.
   */
  @Nullable
  public RouteMarkType getWaitingPoiPickType()
  {
    return mPendingPoiPick == null ? null : mPendingPoiPick.pointType();
  }
}
