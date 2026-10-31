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

  public interface Container
  {
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
     * @param progress progress to be displayed.
     * */
    default void updateBuildProgress(@IntRange(from = 0, to = 100) int progress) {}
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
  @Nullable
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

    // A follow-track build has no route marks (the start point is always null), but core
    // following must stay enabled: disabling it turns the session into RouteNoFollowing, which
    // EnableFollowMode() rejects — on every follow build (initial and each reroute) this would
    // kill starting navigation and the mid-navigation camera follow.
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
      mContainer.updateBuildProgress(mLastBuildProgress);
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

  public void attach(@NonNull Container container)
  {
    mContainer = container;
  }

  public void initialize(@NonNull LocationHelper locationHelper)
  {
    mLocationHelper = locationHelper;
    mLastRouterType = Router.getLastUsed();

    Framework.nativeSetRoutingListener(mRoutingListener);
    Framework.nativeSetRouteProgressListener(mRoutingProgressListener);
    Framework.nativeSetRoutingRecommendationListener(recommendation -> UiThread.run(() -> {
      if (recommendation == RouteRecommendationType.RebuildAfterPointsLoading)
        setStartPoint(locationHelper.getMyPosition());
    }));
    Framework.nativeSetRoutingLoadPointsListener(mRoutingLoadPointsListener);
  }

  public void detach()
  {
    mContainer = null;
  }

  public boolean isAttachedTo(@NonNull Container container)
  {
    return mContainer == container;
  }

  public void detach(@NonNull Container container)
  {
    if (mContainer == container)
      mContainer = null;
  }

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

  public void restoreRoute()
  {
    Framework.nativeLoadRoutePoints();
  }

  public boolean hasSavedRoute()
  {
    return Framework.nativeHasSavedRoutePoints();
  }

  public void saveRoute()
  {
    if (isNavigating() || (isPlanning() && isBuilt()))
      Framework.nativeSaveRoutePoints();
    else if (isPlanning())
      // A restart must not bring back an earlier route of this planning.
      Framework.nativeDeleteSavedRoutePoints();
  }

  public void deleteSavedRoute()
  {
    Framework.nativeDeleteSavedRoutePoints();
  }

  public boolean isRouteSaved()
  {
    return mRouteSaved;
  }

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
    if (isPlanning() && !isFollowingTrack())
      rebuildLastRoute();
  }

  public void rebuildLastRoute()
  {
    setState(State.NONE);
    setBuildState(BuildState.NONE);
    prepare(getStartPoint(), getEndPoint());
  }

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

  public void start()
  {
    // Phone and car actions can outlive the built route they were created for.
    if (!isPlanning() || !isBuilt())
      return;
    Logger.d(TAG, "start");
    final boolean followsTrack = isFollowingTrack();
    if (followsTrack && !Framework.nativeStartFollowedTrack())
    {
      if (Framework.nativeIsRouteBuilding())
        onCoreStartedTrackBuild();
      return;
    }

    // This saving is needed just for situation when the user starts navigation
    // and then app crashes. So, the previous route will be restored on the next app launch.
    saveRoute();

    // Clear stale elevation preview marker.
    Framework.nativeRouteRemoveElevationActivePoint();
    setState(State.NAVIGATION);

    cancelPlanning(false);
    startNavigation();

    if (!followsTrack)
      Framework.nativeFollowRoute();
  }

  /**
   * Starts navigation along a track (the place-page Navigate button): the core follows the
   * track geometry and starts the route build itself, so this method only enters the planning
   * state; the build outcome arrives via the standard routing listener exactly like a
   * {@code nativeBuildRoute}-driven build.
   *
   * @return anything but {@link FollowTrackResult#Ok} leaves the previous session, if any,
   *         untouched.
   */
  @NonNull
  public FollowTrackResult navigateTrack(long trackId)
  {
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
      // The state must leave NAVIGATION before the teardown, exactly like cancel() does:
      // onNavigationCancelled() is the only place that re-syncs the app theme and the map style,
      // and both read isNavigating() -- the auto-dark navigation theme and the Vehicle map style
      // would otherwise stay stuck for the whole follow session (nothing re-syncs them later).
      setState(State.PREPARE);
      cancelNavigation(true);
    }
    // Cache the follow profile, so the planning panel gates on the session's actual router.
    mLastRouterType = Router.getRender();
    enterTrackFollowPlanning();
    return result;
  }

  public boolean isFollowingTrack()
  {
    return Framework.nativeIsFollowingTrack();
  }

  /**
   * Switches the profile a followed track is navigated with (the follow planning header's
   * reduced picker). The core validates before teardown and keeps the choice in this preview.
   *
   * @return anything but {@link FollowTrackResult#Ok} (e.g. the followed track has been deleted
   *         meanwhile) leaves the current session untouched. Taps on the already active profile
   *         and taps while a rebuild is in flight are ignored and report {@code Ok}.
   */
  @NonNull
  public FollowTrackResult setFollowTrackProfile(@NonNull Router router)
  {
    // A follow rebuild (reverse or an earlier profile tap) is still in flight: ignore rapid
    // taps instead of stacking teardown/rebuild cycles.
    if (isBuilding() || router == Router.getRender())
      return FollowTrackResult.Ok;
    Logger.d(TAG, "setFollowTrackProfile: " + router);
    final FollowTrackResult result = FollowTrackResult.valueOf(Framework.nativeSetFollowTrackProfile(router.ordinal()));
    if (result != FollowTrackResult.Ok)
      return result;
    // The core derives the actual follow profile from the selection (it never follows with
    // Transit/Ruler): cache what it routes with, exactly like navigateTrack() does.
    mLastRouterType = Router.getRender();
    enterTrackFollowPlanning();
    return result;
  }

  /**
   * A snapshot of the followed track, or {@code null} when no track is being followed. It stays
   * valid after the track itself has been deleted (navigation then continues on the frozen
   * geometry), so resolving it through the bookmark manager needs an existence check.
   */
  @Nullable
  public FollowedTrackInfo getFollowedTrackInfo()
  {
    return Framework.nativeGetFollowedTrackInfo();
  }

  // The core has already started building the follow-track route (NavigateTrack/FollowTrack):
  // mirror build()'s state entry without issuing another nativeBuildRoute — the build outcome
  // arrives via the standard routing listener.
  private void onCoreStartedTrackBuild()
  {
    mContainsCachedResult = false;
    mCachedRoutingInfo = null;
    mCachedTransitRouteInfo = null;
    mLastBuildProgress = 0;
    mRouteSaved = false;
    setBuildState(BuildState.BUILDING);
    if (mContainer != null)
      mContainer.onStartRouteBuilding();
    updatePlan();
  }

  // There are no route marks while following a track (the track defines the route), so the
  // planning panel is shown point-less.
  private void enterTrackFollowPlanning()
  {
    setState(State.PREPARE);
    onCoreStartedTrackBuild();
    startPlanning(null, null);
  }

  // Route marks must never land inside a track-follow session: the follow has no editable
  // points, so a mark would neither affect the followed route nor be torn down with it — a
  // hybrid session with an orphan mark. A "route from/to" tap instead leaves the follow through
  // the standard prepare() path (its cancel() closes the routing session — the core's single
  // follow-teardown seam) and opens a fresh normal plan with the tapped point.
  private void leaveTrackFollow(@Nullable MapObject startPoint, @Nullable MapObject endPoint)
  {
    Logger.d(TAG, "leaveTrackFollow");
    // navigateTrack() cached the follow profile in mLastRouterType to gate the follow planning
    // panel; the fresh normal plan must run on the user's own persisted selection instead —
    // prepare() would otherwise adopt and persist the follow profile via Router.set().
    mLastRouterType = Router.getLastUsed();
    prepare(startPoint, endPoint);
  }

  public void replaceStop(@NonNull MapObject mapObject)
  {
    final PendingPoiPick pick = requirePendingPoiPick();
    if (!pick.isReplacement())
      throw new IllegalStateException("A route point replacement was not requested");
    replaceRoutePoint(pick.pointType(), mapObject, pick.replaceStopIndex());
    finalizeStopChange();
  }

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

  // With no pick armed the mode is SET, which is the plain place page path: insert an intermediate point.
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

  public void removeStop(@NonNull MapObject mapObject)
  {
    RoutePointInfo info = mapObject.getRoutePointInfo();
    if (info == null)
      throw new AssertionError("A stop point must have the route point info!");

    removeStop(info.mMarkType, info.mIntermediateIndex);
  }

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
   * @return False if not navigating, true otherwise
   */
  public boolean resetToPlanningStateIfNavigating()
  {
    if (isNavigating())
    {
      final FollowedTrackInfo track = getFollowedTrackInfo();
      if (track != null)
      {
        // A geometric follow is rebuilt by re-invoking it with the same direction: build() would
        // exit the follow mode through nativeBuildRoute. The followed track may have been deleted
        // meanwhile (navigation continues on a frozen snapshot) — stop the session entirely then.
        if (FollowTrackResult.valueOf(Framework.nativeFollowTrack(track.getTrackId(), track.isReversed()))
            != FollowTrackResult.Ok)
          return cancel();
        onCoreStartedTrackBuild();
      }
      else
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

    // The router type a track session cached in mLastRouterType (the follow profile) is
    // session-only -- the core deliberately does not persist it.
    // Restore the user's own selection, otherwise the next single-point prepare() (e.g. "route
    // from" a POI, where initLastRouteType() has nothing to derive from) would persist that type
    // through Router.set() and replace a selection the user never changed. A no-op for every
    // normal session, where Router.set() has persisted mLastRouterType already.
    mLastRouterType = Router.getLastUsed();

    setBuildState(BuildState.NONE);
    setState(State.NONE);

    // Clear stale elevation preview marker.
    Framework.nativeRouteRemoveElevationActivePoint();
    if (deleteSavedRoute)
      Framework.nativeDeleteSavedRoutePoints();
    Framework.nativeCloseRouting();
  }

  public boolean cancel()
  {
    return cancel(true);
  }

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
    return getLastRouterType() == Router.Transit;
  }

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

  public void waitForPoiPick(@NonNull RouteMarkType pointType)
  {
    if (pointType == RouteMarkType.Intermediate)
      throw new IllegalArgumentException("A stop is picked with waitForPoiPickToAppend()");
    armPoiPick(pointType, PoiPickMode.SET, -1);
  }

  public void waitForPoiReplacement(@NonNull RouteMarkType pointType, int index)
  {
    armPoiPick(pointType, PoiPickMode.REPLACE, index);
  }

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

  // A stop pick commits through the place page's Add/Replace button; a pick for an empty start or finish slot uses
  // the regular routing buttons.
  public boolean isWaitingStopPick()
  {
    return getPoiPickMode() != PoiPickMode.SET;
  }

  // A commit applies its pick before it closes the search or the map chooser, so this only ever drops a pick that is
  // already applied or was abandoned.
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
    if (point != null && isFollowingTrack())
    {
      // Mirrors the place-page "route from" action outside a planning session.
      leaveTrackFollow(point, null /* endPoint */);
      return true;
    }
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
    if (point != null && isFollowingTrack())
    {
      // Mirrors the "route to" action outside a planning session: start from the current
      // position when there is one (MwmActivity.startLocationToPoint()).
      leaveTrackFollow(mLocationHelper != null ? mLocationHelper.getMyPosition() : null, point);
      return true;
    }
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

  public void reverseRoute()
  {
    Logger.d(TAG, "reverseRoute");

    if (Framework.nativeReverseRoutePoints())
      build();
  }

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

  @Nullable
  public RouteMarkType getWaitingPoiPickType()
  {
    return mPendingPoiPick == null ? null : mPendingPoiPick.pointType();
  }
}
