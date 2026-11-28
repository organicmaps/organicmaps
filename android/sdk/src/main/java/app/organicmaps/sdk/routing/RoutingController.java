package app.organicmaps.sdk.routing;

import android.text.TextUtils;
import androidx.annotation.IntRange;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.VisibleForTesting;
import androidx.core.util.Pair;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.DataChangedListener;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.util.Assert;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.util.log.Logger;
import org.chromium.base.ObserverList;

/**
 * Main-thread, process-wide route planning and navigation coordinator.
 * Obtain it through {@link #get} after SDK initialization. Attach one {@link Container} for host UI callbacks,
 * call {@link #restore} when that host becomes active, and detach it when the host stops. The controller
 * retains route state across activity recreation; it does not provide the host foreground service or UI.
 * Detailed route building requires all relevant region maps. Missing-map errors are reported to the container.
 * Wait for {@link app.organicmaps.sdk.MapsReadiness} before preparing, editing, building or restoring a route.
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
   * Hosts that display route planning implement {@link #showRoutePlan}; route builds run independently of the UI.
   * Other default methods do nothing, allowing a host to implement only the controls it displays.
   */
  public interface Container
  {
    /**
     * Shows or hides planning UI. Route building does not wait for the UI to be ready.
     *
     * @param show whether planning UI should be shown
     */
    default void showRoutePlan(boolean show) {}
    default void showNavigation(boolean show) {}
    default void updateMenu() {}
    default void onNavigationCancelled() {}
    default void onNavigationStarted() {}
    /**
     * A track Start was accepted, but navigation begins only after the core refreshes the approach to the track,
     * possibly once this host has stopped. A host that owns a foreground service should start it now.
     * If the Start is dropped instead, {@link NavigationStateListener}s are told navigation is inactive.
     */
    default void onNavigationStartPending() {}
    default void onPlanningCancelled() {}
    default void onPlanningStarted() {}
    default void onAddedStop() {}
    default void onRemovedStop() {}
    default void onStopPointLimitReached() {}
    default void onPoiPickCompleted() {}
    default void onResetToPlanningState() {}
    default void onBuiltRoute() {}
    default void onRouteReadyToAutoStart() {}
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
   * Android Auto, notification stop, or route finished, and with the UI host detached. Also reports
   * {@code false} when a pending track Start is dropped, see {@link Container#onNavigationStartPending}.
   */
  public interface NavigationStateListener
  {
    void onNavigationStateChanged(boolean active);
  }

  private static final RoutingController sInstance = new RoutingController();

  @Nullable
  private Container mContainer;
  // The SDK initializes the controller on the same call stack right before it publishes maps readiness.
  private boolean mInitialized;
  private LocationHelper mLocationHelper;

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
  // Start was accepted for a track preview whose approach the core is refreshing.
  private boolean mTrackStartPending;
  // Navigation started while no host could present it; restore() hands it over.
  private boolean mNavigationStartUnpresented;
  private boolean mHasContainerSavedState;
  private boolean mContainsCachedResult;
  private int mLastResultCode;
  private String[] mLastMissingMaps;
  @Nullable
  private RoutingInfo mCachedRoutingInfo;
  @Nullable
  private TransitRouteInfo mCachedTransitRouteInfo;

  private boolean mRouteSaved;
  private boolean mStartNavigationAfterBuild;

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

      final boolean built = mLastResultCode == ResultCodes.NO_ERROR || mLastResultCode == ResultCodes.HAS_WARNINGS;
      if (built)
        onBuiltRoute();
      else
        mStartNavigationAfterBuild = false;

      // The core follows a track once a pending Start's approach is ready. Navigation observers must see it even
      // while the host is stopped; the host presents it in restore().
      if (built && isPlanning() && isFollowingTrack() && isRoutingFollowing())
        enterNavigation();
      else
        dropPendingTrackStart();

      processRoutingEvent();
    }
  };

  @VisibleForTesting
  @Nullable
  RoutingInfo fetchRouteFollowingInfo()
  {
    return Framework.nativeGetRouteFollowingInfo();
  }

  private void onBuiltRoute()
  {
    mCachedRoutingInfo = fetchRouteFollowingInfo();
    if (isTransitType())
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

  // Bookmark changes are published from inside Core's edit session; teardown must wait for it to finish.
  private final DataChangedListener mBookmarksListener = () -> UiThread.runLater(this::cancelIfFollowedTrackDeleted);

  // Deleting the source track ends its route: the preview, a pending Start and navigation alike.
  private void cancelIfFollowedTrackDeleted()
  {
    if (mState == State.NONE)
      return;
    final FollowedTrackInfo track = getFollowedTrackInfo();
    if (track != null && !BookmarkManager.INSTANCE.hasTrack(track.getTrackId()))
      cancel();
  }

  public static RoutingController get()
  {
    return sInstance;
  }

  private void checkMapsReady()
  {
    if (!mInitialized)
      throw new IllegalStateException("Wait for SDK maps to load before issuing routing commands");
  }

  private void processRoutingEvent()
  {
    final Container container = mContainer;
    if (!mContainsCachedResult || container == null || mHasContainerSavedState)
      return;

    mContainsCachedResult = false;

    if (mStartNavigationAfterBuild
        && (mLastResultCode == ResultCodes.NO_ERROR || mLastResultCode == ResultCodes.HAS_WARNINGS))
    {
      mStartNavigationAfterBuild = false;
      container.onRouteReadyToAutoStart();
      if (mContainer != container)
        return;
    }

    if (mLastResultCode == ResultCodes.NO_ERROR || mLastResultCode == ResultCodes.HAS_WARNINGS)
    {
      if (mLastResultCode == ResultCodes.HAS_WARNINGS)
        container.onDrivingOptionsWarning();
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
      notifyNavigationState(isNavigating);

    if (mContainer != null)
      mContainer.updateMenu();
  }

  private void notifyNavigationState(boolean active)
  {
    for (final NavigationStateListener listener : mNavigationStateListeners)
      listener.onNavigationStateChanged(active);
  }

  // The host may have started its navigation service for this Start.
  private void dropPendingTrackStart()
  {
    if (!mTrackStartPending)
      return;
    mTrackStartPending = false;
    notifyNavigationState(false);
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

    // Track previews have no route marks; disabling following would prevent Start.
    if (mBuildState == BuildState.BUILT && !isFollowingTrack())
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
      mContainer.updateBuildProgress(mLastBuildProgress, getLastRouterType());
  }

  private void showRoutePlan()
  {
    if (mContainer != null)
      mContainer.showRoutePlan(true);
  }

  /**
   * Replaces the single UI container. Call {@link #restore} to replay its current state after attachment.
   */
  public void attach(@NonNull Container container)
  {
    mContainer = container;
  }

  /**
   * Reads the installed profile and registers routing callbacks after native routing initialization.
   * Called automatically before the SDK publishes maps readiness.
   */
  public void initialize(@NonNull LocationHelper locationHelper)
  {
    mLocationHelper = locationHelper;
    mLastRouterType = Router.get();

    Framework.nativeSetRoutingListener(mRoutingListener);
    Framework.nativeSetRouteProgressListener(mRoutingProgressListener);
    Framework.nativeSetRoutingRecommendationListener(recommendation -> UiThread.run(() -> {
      if (recommendation == RouteRecommendationType.RebuildAfterPointsLoading)
        setStartPoint(locationHelper.getMyPosition());
    }));
    Framework.nativeSetTrackEntryTapListener(() -> UiThread.run(this::onCoreStartedTrackBuild));
    BookmarkManager.INSTANCE.addCategoriesUpdatesListener(mBookmarksListener);
    mInitialized = true;
  }

  /**
   * Releases the calling UI container without cancelling routing. A stale host's lifecycle callback
   * may arrive after a newer host attaches, so only the attached container may release itself.
   */
  public void detach(@NonNull Container container)
  {
    if (mContainer == container)
      mContainer = null;
  }

  /**
   * Replays planning, navigation, build progress, and any deferred result to the attached container,
   * including a navigation start that happened while no host could present it.
   * Call after attaching a resumed host; this also clears the saved-UI-state flag.
   */
  @MainThread
  public void restore()
  {
    final Container container = mContainer;
    mHasContainerSavedState = false;
    if (container == null)
      return;

    if (!isNavigating())
      mNavigationStartUnpresented = false;
    final boolean presentStart = mNavigationStartUnpresented;
    // Presentation callbacks can replace this host; its successor must restore its own UI and result.
    if (isPlanning() || presentStart)
    {
      container.showRoutePlan(isPlanning());
      if (mContainer != container)
        return;
    }
    container.showNavigation(isNavigating());
    if (mContainer != container)
      return;
    if (presentStart)
    {
      mNavigationStartUnpresented = false;
      container.onNavigationStarted();
      if (mContainer != container)
        return;
    }
    container.updateMenu();
    if (mContainer != container)
      return;
    updateProgress();
    if (mContainer == container)
      processRoutingEvent();
  }

  /**
   * Defers UI results while the calling host has saved its state; {@link #restore} resumes processing.
   * A stale host's lifecycle callback must not defer results for a newer attached host.
   */
  public void onSaveState(@NonNull Container container)
  {
    if (mContainer == container)
      mHasContainerSavedState = true;
  }

  @VisibleForTesting
  RoutingListener getRoutingListenerForTesting()
  {
    return mRoutingListener;
  }

  @VisibleForTesting
  void markInitializedForTesting()
  {
    mInitialized = true;
  }

  @VisibleForTesting
  void build()
  {
    Logger.d(TAG, "build");
    beginBuild(false);
    Framework.nativeBuildRoute();
  }

  private void beginBuild(boolean startNavigation)
  {
    mStartNavigationAfterBuild = startNavigation;
    mContainsCachedResult = false;
    mLastBuildProgress = 0;
    mRouteSaved = false;
    // This build uses the current settings and route marks, so it consumes any deferred options change.
    mPendingOptionsRebuild = false;

    setBuildState(BuildState.BUILDING);
    if (mContainer != null)
      mContainer.onStartRouteBuilding();

    updatePlan();
  }

  /**
   * Restores saved route points synchronously and starts an asynchronous build when restoration succeeds.
   */
  public void restoreRoute()
  {
    checkMapsReady();
    // Recovery preserves the user's last-used router.
    if (Framework.nativeLoadRoutePoints())
      rebuildLastRoute();
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
    checkMapsReady();
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
   * Returns to planning and rebuilds the existing native itinerary with the selected profile and settings.
   * Does nothing for a followed track: road-route settings and stale actions cannot replace its captured entry.
   */
  public void rebuildLastRoute()
  {
    checkMapsReady();
    if (isFollowingTrack())
      return;
    mStartNavigationAfterBuild = false;
    mContainsCachedResult = false;
    setState(State.PREPARE);
    setBuildState(BuildState.NONE);
    startPlanning(getStartPoint(), getEndPoint());
  }

  /**
   * Starts a new route plan and selects a suggested profile for the endpoints.
   * @param startPoint origin, or {@code null} to leave it unresolved
   * @param endPoint destination, or {@code null} to leave it unresolved
   */
  public void prepare(@Nullable MapObject startPoint, @Nullable MapObject endPoint)
  {
    checkMapsReady();
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
    checkMapsReady();
    if (!cancel())
    {
      Framework.nativeRemoveRoutePoints();
      Framework.nativeDeleteSavedRoutePoints();
    }
    setState(State.PREPARE);

    mLastRouterType = routerType;
    Router.set(mLastRouterType);

    if (startPoint != null || endPoint != null)
      setPointsInternal(startPoint, endPoint);

    startPlanning(startPoint, endPoint);
  }

  /**
   * API-route entry: the core owns the parsed itinerary (order, callbacks, optimization,
   * start direction) and materializes it via ExecuteRouteApiRequest; here we only drive
   * UI state and honor the explicit navigation intent.
   */
  public void prepareApiRoute(@NonNull Router routerType, boolean startNavigation)
  {
    checkMapsReady();
    // The native API request replaces the marks and retires the previous recovery together.
    cancel(false /* deleteSavedRoute */, false /* closeNativeRoute */);
    setState(State.PREPARE);

    mLastRouterType = routerType;
    Router.set(mLastRouterType);
    // Build validation can notify the routing UI during the native call.
    showRoutePlan();
    beginBuild(startNavigation);
    Framework.nativeExecuteRouteApiRequest();

    if (isPlanning() && mContainer != null)
    {
      mContainer.updateMenu();
      mContainer.onPlanningStarted();
    }
  }

  /**
   * Starts following a successfully built planned route; otherwise does nothing.
   * A track approach refresh keeps the plan open until the core enters following.
   * Requires a created map engine ({@link app.organicmaps.sdk.Map#isEngineCreated()}). The host supplies location
   * updates and any foreground-service lifecycle required for navigation.
   */
  public void start()
  {
    // Phone and car actions can outlive the built route they were created for.
    if (!canStartNavigation())
      return;
    Logger.d(TAG, "start");
    final boolean followsTrack = isFollowingTrack();
    if (followsTrack)
    {
      if (!startFollowedTrack())
        return;
      if (!isRoutingFollowing())
      {
        mTrackStartPending = true;
        onCoreStartedTrackBuild();
        if (mContainer != null)
          mContainer.onNavigationStartPending();
        return;
      }
    }

    enterNavigation();
    if (!followsTrack)
      Framework.nativeFollowRoute();
  }

  @VisibleForTesting
  boolean startFollowedTrack()
  {
    return Framework.nativeStartFollowedTrack();
  }

  @VisibleForTesting
  boolean isRoutingFollowing()
  {
    return Framework.nativeIsRoutingFollowing();
  }

  private void enterNavigation()
  {
    mTrackStartPending = false;
    prepareNavigation();
    setState(State.NAVIGATION);

    // A stopped host cannot change its UI; restore() presents the start instead.
    mNavigationStartUnpresented = mContainer == null || mHasContainerSavedState;
    if (mNavigationStartUnpresented)
      return;
    cancelPlanning(false);
    startNavigation();
  }

  @VisibleForTesting
  void prepareNavigation()
  {
    // This saving is needed just for situation when the user starts navigation
    // and then app crashes. So, the previous route will be restored on the next app launch.
    saveRoute();

    // Clear stale elevation preview marker.
    Framework.nativeRouteRemoveElevationActivePoint();
  }

  /**
   * Captures the selected track section and opens planning. The core reports its asynchronous
   * preview build through the normal routing listener. Navigation begins through {@link #start}.
   *
   * @return anything but {@link FollowTrackResult#Ok} leaves the previous session, if any,
   *         untouched.
   */
  @NonNull
  public FollowTrackResult navigateTrack(long trackId)
  {
    checkMapsReady();
    Logger.d(TAG, "navigateTrack: " + trackId);
    // The core validates the track before tearing anything down, so it goes first: a refusal
    // leaves the previous session fully usable.
    final FollowTrackResult result = FollowTrackResult.valueOf(Framework.nativeNavigateTrack(trackId));
    if (result != FollowTrackResult.Ok)
      return result;
    // The core has already closed the previous session as part of NavigateTrack; cancel() here
    // would close the just-started build with it (nativeCloseRouting), so only the Java-side
    // session state is reset.
    resetPoiPickState();
    if (isPlanning() || isNavigating())
      Framework.nativeRouteRemoveElevationActivePoint();
    if (isNavigating())
    {
      // Leave NAVIGATION before the teardown, like cancel(): onNavigationCancelled() re-syncs the app theme and
      // the map style from isNavigating().
      setState(State.PREPARE);
      cancelNavigation(true);
    }
    enterTrackFollowPlanning();
    return result;
  }

  /**
   * Reverses a completed preview at the selected location. A closed trailhead switches chart endpoints.
   * Requires {@link #isFollowingTrack()}.
   * Returns {@link FollowTrackResult#Busy} while building or navigating.
   */
  @NonNull
  public FollowTrackResult reverseFollowedTrack()
  {
    final FollowTrackResult result = FollowTrackResult.valueOf(Framework.nativeReverseFollowedTrack());
    if (result == FollowTrackResult.Ok)
      onCoreStartedTrackBuild();
    return result;
  }

  public boolean isFollowingTrack()
  {
    return Framework.nativeIsFollowingTrack();
  }

  /**
   * Switches the profile a followed track is navigated with; requires {@link #isFollowingTrack()}. Only
   * {@link Router#Vehicle}, {@link Router#Bicycle} and {@link Router#Pedestrian} are supported. The core validates
   * before teardown and keeps the choice in this preview. A preview build in flight is replaced.
   *
   * @return anything but {@link FollowTrackResult#Ok} leaves the current session untouched, e.g.
   *         {@link FollowTrackResult#Busy} for a pending Start. A request for the active profile is ignored and
   *         reports {@code Ok}.
   */
  @NonNull
  public FollowTrackResult setFollowTrackProfile(@NonNull Router router)
  {
    Assert.debug(isFollowingTrack(), "Switching the follow profile without a followed track");
    if (router == Router.getRender())
      return FollowTrackResult.Ok;
    Logger.d(TAG, "setFollowTrackProfile: " + router);
    final FollowTrackResult result = FollowTrackResult.valueOf(Framework.nativeSetFollowTrackProfile(router.getType()));
    if (result == FollowTrackResult.Ok)
      onCoreStartedTrackBuild();
    return result;
  }

  /**
   * Rebuilds from a finite distance along the full track, preserving profile and viewport.
   * Keeps the chosen direction unless only the opposite side of the entry has geometry.
   * Requires {@link #isFollowingTrack()}. Refusals leave the preview and selection unchanged.
   */
  @NonNull
  public FollowTrackResult updateFollowedTrackEntryAt(double distanceMeters)
  {
    final FollowTrackResult result =
        FollowTrackResult.valueOf(Framework.nativeUpdateFollowedTrackEntryAt(distanceMeters));
    if (result == FollowTrackResult.Ok)
      onCoreStartedTrackBuild();
    return result;
  }

  /**
   * A snapshot of the followed track, or {@code null} when no track is being followed. Later edits of the track don't
   * change it. Deleting the track ends the route once the bookmark change is applied, so resolving the snapshot
   * through the bookmark manager still needs an existence check.
   */
  @Nullable
  public FollowedTrackInfo getFollowedTrackInfo()
  {
    return Framework.nativeGetFollowedTrackInfo();
  }

  // Mirror a core-started track build; its result uses the standard routing listener.
  private void onCoreStartedTrackBuild()
  {
    resetForTrackBuild();
    updatePlan();
  }

  private void resetForTrackBuild()
  {
    mStartNavigationAfterBuild = false;
    mContainsCachedResult = false;
    mCachedRoutingInfo = null;
    mCachedTransitRouteInfo = null;
    mLastBuildProgress = 0;
    mRouteSaved = false;
    setBuildState(BuildState.BUILDING);
    if (mContainer != null)
      mContainer.onStartRouteBuilding();
  }

  // A followed track has no route marks (the track defines the route), so planning starts
  // point-less and the planning panel shows fixed rows for the followed track instead.
  private void enterTrackFollowPlanning()
  {
    setState(State.PREPARE);
    // startPlanning() publishes the initial progress once the plan is shown.
    resetForTrackBuild();
    startPlanning(null, null);
  }

  /**
   * Commits a place to a previously armed point replacement, then rebuilds the route.
   * @throws IllegalStateException if no replacement was requested
   */
  public void replaceStop(@NonNull MapObject mapObject)
  {
    checkMapsReady();
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
    checkMapsReady();
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
    checkMapsReady();
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
    checkMapsReady();
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
    checkMapsReady();
    build();
    setState(State.PREPARE);
    showRoutePlan();
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
      if (isFollowingTrack())
      {
        final FollowTrackResult result = resetFollowedTrackToPlanning();
        Assert.debug(result == FollowTrackResult.Ok, "Unexpected track reset result: " + result);
        onCoreStartedTrackBuild();
      }
      else
        build();
      transitionToPlanning();
      return true;
    }
    return false;
  }

  @VisibleForTesting
  @NonNull
  FollowTrackResult resetFollowedTrackToPlanning()
  {
    return FollowTrackResult.valueOf(Framework.nativeResetFollowedTrackToPlanning());
  }

  // Each caller has already requested a route build; this only changes the navigation and planning UI.
  private void transitionToPlanning()
  {
    setState(State.PREPARE);
    cancelNavigation(false);
    showRoutePlan();
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

  private void cancelInternal(boolean deleteSavedRoute, boolean closeNativeRoute)
  {
    mContainsCachedResult = false;
    Logger.d(TAG, "cancelInternal");

    resetPoiPickState();
    dropPendingTrackStart();
    mStartNavigationAfterBuild = false;

    setBuildState(BuildState.NONE);
    setState(State.NONE);

    // Clear stale elevation preview marker.
    Framework.nativeRouteRemoveElevationActivePoint();
    if (deleteSavedRoute)
      Framework.nativeDeleteSavedRoutePoints();
    if (closeNativeRoute)
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
    return cancel(deleteSavedRoute, true /* closeNativeRoute */);
  }

  private boolean cancel(boolean deleteSavedRoute, boolean closeNativeRoute)
  {
    if (isPlanning())
    {
      Logger.d(TAG, "cancel: planning");

      cancelInternal(deleteSavedRoute, closeNativeRoute);
      cancelPlanning(true);
      return true;
    }

    if (isNavigating())
    {
      Logger.d(TAG, "cancel: navigating");

      cancelInternal(deleteSavedRoute, closeNativeRoute);
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

  private void startPlanning(final @Nullable MapObject startPoint, final @Nullable MapObject endPoint)
  {
    showRoutePlan();
    if (startPoint != null && endPoint != null)
      build();
    else
      updatePlan();
    if (mContainer != null)
      mContainer.onPlanningStarted();
  }

  private void cancelPlanning(boolean fireEvent)
  {
    if (mContainer != null)
    {
      mContainer.showRoutePlan(false);
      if (fireEvent)
        mContainer.onPlanningCancelled();
    }
  }

  @VisibleForTesting
  void startNavigation()
  {
    final Container container = mContainer;
    if (container != null)
    {
      container.showNavigation(true);
      // The planning host can be destroyed before its navigation successor attaches.
      if (mContainer == container)
        container.onNavigationStarted();
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
    return getLastRouterType() == Router.Transit;
  }

  /**
   * Returns whether the selected profile is driving, independently of navigation state.
   */
  public boolean isVehicleRouterType()
  {
    return getLastRouterType() == Router.Vehicle;
  }

  public boolean isRulerRouterType()
  {
    return getLastRouterType() == Router.Ruler;
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

  public boolean canStartNavigation()
  {
    if (!isPlanning() || !isBuilt())
      return false;
    if (isFollowingTrack())
      return true;
    final MapObject startPoint = getStartPoint();
    return startPoint != null && startPoint.isMyPosition();
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
    checkMapsReady();
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
  public boolean setStartPoint(@Nullable MapObject point)
  {
    checkMapsReady();
    final boolean result = setEndpoint(RouteMarkType.Start, point);
    finalizePendingPoiPick();
    return result;
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
  public boolean setEndPoint(@Nullable MapObject point)
  {
    checkMapsReady();
    final boolean result = setEndpoint(RouteMarkType.Finish, point);
    finalizePendingPoiPick();
    return result;
  }

  private boolean setEndpoint(@NonNull RouteMarkType type, @Nullable MapObject point)
  {
    if (point == null)
      return false;
    if (isFollowingTrack())
    {
      // A new road endpoint replaces the captured track plan.
      if (type == RouteMarkType.Start)
        prepare(point, null);
      else
        prepare(mLocationHelper.getMyPosition(), point);
      return true;
    }
    final MapObject current = type == RouteMarkType.Start ? getStartPoint() : getEndPoint();
    if (MapObject.same(current, point))
      return false;
    final MapObject opposite = type == RouteMarkType.Start ? getEndPoint() : getStartPoint();
    if (point.sameAs(opposite))
    {
      if (current == null)
        return false;
      swapPoints();
      return true;
    }
    addRoutePoint(type, point);
    if (mContainer != null)
      mContainer.updateMenu();
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
    checkMapsReady();
    Logger.d(TAG, "swapPoints");

    final int count = Framework.nativeGetRoutePoints().length;
    if (count < 2)
      return;
    // Keep stops in order and preserve metadata owned by the core when exchanging endpoints.
    Framework.nativeMoveRoutePoint(0, count - 1);
    Framework.nativeMoveRoutePoint(count - 2, 0);
    checkAndBuildRoute();
    if (mContainer != null)
      mContainer.updateMenu();
  }

  /**
   * Reverses the full route-point order, including intermediate stops, and rebuilds.
   */
  public void reverseRoute()
  {
    checkMapsReady();
    Logger.d(TAG, "reverseRoute");

    if (Framework.nativeReverseRoutePoints())
      build();
  }

  /**
   * Changes the routing profile and rebuilds when the profile differs.
   */
  public void setRouterType(Router router)
  {
    checkMapsReady();
    Logger.d(TAG, "setRouterType: " + mLastRouterType + " -> " + router);

    // Nothing to rebuild when the already selected router type is tapped again.
    if (router == mLastRouterType)
      return;

    // A router change closes the core's track session; end it here too, so no stale planning or navigation remains.
    if (isFollowingTrack())
      cancel();

    mLastRouterType = router;
    Router.set(router);

    if (getStartPoint() != null && getEndPoint() != null)
      build();
  }

  /**
   * The router type to present and gate on. While following a track it is the render router
   * type — the follow profile the core routes with, never Transit/Ruler, so e.g. the Start
   * button stays enabled — and the user's own selection ({@code mLastRouterType}) is left
   * untouched to survive the track session.
   */
  public Router getLastRouterType()
  {
    if (isFollowingTrack())
      return Router.getRender();
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
