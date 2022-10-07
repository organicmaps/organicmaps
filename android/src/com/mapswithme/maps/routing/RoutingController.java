package com.mapswithme.maps.routing;

import android.content.Context;
import android.content.DialogInterface;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.DimenRes;
import androidx.annotation.IntRange;
import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AlertDialog;
import androidx.core.util.Pair;
import androidx.fragment.app.FragmentActivity;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.Initializable;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.widget.placepage.CoordinatesFormat;
import com.mapswithme.util.Config;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;

import java.util.Calendar;
import java.util.concurrent.TimeUnit;


@androidx.annotation.UiThread
public class RoutingController implements Initializable<Void>
{
  private static final String TAG = RoutingController.class.getSimpleName();

  private enum State
  {
    NONE,
    PREPARE,
    NAVIGATION
  }

  enum BuildState
  {
    NONE,
    BUILDING,
    BUILT,
    ERROR
  }

  public interface Container
  {
    FragmentActivity requireActivity();
    void showSearch();
    void showRoutePlan(boolean show, @Nullable Runnable completionListener);
    void showNavigation(boolean show);
    void showDownloader(boolean openDownloaded);
    void updateMenu();
    void onNavigationCancelled();
    void onNavigationStarted();
    void onPlanningCancelled();
    void onPlanningStarted();
    void onAddedStop();
    void onRemovedStop();
    void onResetToPlanningState();
    void onBuiltRoute();
    void onDrivingOptionsWarning();
    boolean isSubwayEnabled();
    void onCommonBuildError(int lastResultCode, @NonNull String[] lastMissingMaps);
    void onDrivingOptionsBuildError();

    /**
     * @param progress progress to be displayed.
     * */
    void updateBuildProgress(@IntRange(from = 0, to = 100) int progress, @Framework.RouterType int router);
    void onStartRouteBuilding();
  }

  private static final int NO_WAITING_POI_PICK = -1;
  private static final RoutingController sInstance = new RoutingController();

  @Nullable
  private Container mContainer;

  private BuildState mBuildState = BuildState.NONE;
  private State mState = State.NONE;
  //@RoutePointInfo.RouteMarkType
  private int mWaitingPoiPickType = NO_WAITING_POI_PICK;
  private int mLastBuildProgress;
  @Framework.RouterType
  private int mLastRouterType;

  private boolean mHasContainerSavedState;
  private boolean mContainsCachedResult;
  private int mLastResultCode;
  private String[] mLastMissingMaps;
  @Nullable
  private RoutingInfo mCachedRoutingInfo;
  @Nullable
  private TransitRouteInfo mCachedTransitRouteInfo;

  private int mInvalidRoutePointsTransactionId;
  private int mRemovingIntermediatePointsTransactionId;

  @SuppressWarnings("FieldCanBeLocal")
  private final Framework.RoutingListener mRoutingListener = new Framework.RoutingListener()
  {
    @MainThread
    @Override
    public void onRoutingEvent(final int resultCode, @Nullable final String[] missingMaps)
    {
      Logger.d(TAG, "onRoutingEvent(resultCode: " + resultCode + ")");
      mLastResultCode = resultCode;
      mLastMissingMaps = missingMaps;
      mContainsCachedResult = true;

      if (mLastResultCode == ResultCodesHelper.NO_ERROR
          || ResultCodesHelper.isMoreMapsNeeded(mLastResultCode))
      {
        onBuiltRoute();
      }
      else if (mLastResultCode == ResultCodesHelper.HAS_WARNINGS)
      {
        onBuiltRoute();
        if (mContainer != null)
          mContainer.onDrivingOptionsWarning();
      }

      processRoutingEvent();
    }
  };
  private void onBuiltRoute()
  {
    mCachedRoutingInfo = Framework.nativeGetRouteFollowingInfo();
    if (mLastRouterType == Framework.ROUTER_TYPE_TRANSIT)
      mCachedTransitRouteInfo = Framework.nativeGetTransitRouteInfo();
    setBuildState(BuildState.BUILT);
    mLastBuildProgress = 100;
    if (mContainer != null)
      mContainer.onBuiltRoute();
  }

  @SuppressWarnings("FieldCanBeLocal")
  private final Framework.RoutingProgressListener mRoutingProgressListener = new Framework.RoutingProgressListener()
  {
    @MainThread
    @Override
    public void onRouteBuildingProgress(float progress)
    {
      mLastBuildProgress = (int) progress;
      updateProgress();
    }
  };

  @SuppressWarnings("FieldCanBeLocal")
  private final Framework.RoutingRecommendationListener mRoutingRecommendationListener =
    recommendation -> UiThread.run(() -> {
      if (recommendation == Framework.ROUTE_REBUILD_AFTER_POINTS_LOADING)
        setStartPoint(LocationHelper.INSTANCE.getMyPosition());
    });

  @SuppressWarnings("FieldCanBeLocal")
  private final Framework.RoutingLoadPointsListener mRoutingLoadPointsListener =
    success -> {
      if (success)
        prepare(getStartPoint(), getEndPoint());
    };

  public static RoutingController get()
  {
    return sInstance;
  }

  private void processRoutingEvent()
  {
    if (!mContainsCachedResult ||
        mContainer == null ||
        mHasContainerSavedState)
      return;

    mContainsCachedResult = false;

    if (isDrivingOptionsBuildError())
      mContainer.onDrivingOptionsWarning();

    if (mLastResultCode == ResultCodesHelper.NO_ERROR || mLastResultCode == ResultCodesHelper.HAS_WARNINGS)
    {
      updatePlan();
      return;
    }

    if (mLastResultCode == ResultCodesHelper.CANCELLED)
    {
      setBuildState(BuildState.NONE);
      updatePlan();
      return;
    }

    if (!ResultCodesHelper.isMoreMapsNeeded(mLastResultCode))
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
    return !ResultCodesHelper.isMoreMapsNeeded(mLastResultCode) && isVehicleRouterType()
           && RoutingOptions.hasAnyOptions();
  }

  private void setState(State newState)
  {
    Logger.d(TAG, "[S] State: " + mState + " -> " + newState + ", BuildState: " + mBuildState);
    mState = newState;

    if (mContainer != null)
      mContainer.updateMenu();
  }

  private void setBuildState(BuildState newState)
  {
    Logger.d(TAG, "[B] State: " + mState + ", BuildState: " + mBuildState + " -> " + newState);
    mBuildState = newState;

    if (mBuildState == BuildState.BUILT && !MapObject.isOfType(MapObject.MY_POSITION, getStartPoint()))
      Framework.nativeDisableFollowing();

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

  public void attach(@NonNull Container container)
  {
    mContainer = container;
  }

  @Override
  public void initialize(@Nullable Void aVoid)
  {
    mLastRouterType = Framework.nativeGetLastUsedRouter();
    mInvalidRoutePointsTransactionId = Framework.nativeInvalidRoutePointsTransactionId();
    mRemovingIntermediatePointsTransactionId = mInvalidRoutePointsTransactionId;

    Framework.nativeSetRoutingListener(mRoutingListener);
    Framework.nativeSetRouteProgressListener(mRoutingProgressListener);
    Framework.nativeSetRoutingRecommendationListener(mRoutingRecommendationListener);
    Framework.nativeSetRoutingLoadPointsListener(mRoutingLoadPointsListener);
  }

  @Override
  public void destroy()
  {
    // No op.
  }

  public void detach()
  {
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

    setBuildState(BuildState.BUILDING);
    if (mContainer != null)
      mContainer.onStartRouteBuilding();

    updatePlan();

    Framework.nativeBuildRoute();
  }

  private void showDisclaimer(final MapObject startPoint, final MapObject endPoint,
                              final boolean fromApi)
  {
    if (mContainer == null)
      return;

    FragmentActivity activity = mContainer.requireActivity();
    StringBuilder builder = new StringBuilder();
    for (int resId : new int[] { R.string.dialog_routing_disclaimer_priority, R.string.dialog_routing_disclaimer_precision,
                                 R.string.dialog_routing_disclaimer_recommendations, R.string.dialog_routing_disclaimer_borders,
                                 R.string.dialog_routing_disclaimer_beware })
      builder.append(MwmApplication.from(activity.getApplicationContext()).getString(resId)).append("\n\n");

    new AlertDialog.Builder(activity)
        .setTitle(R.string.dialog_routing_disclaimer_title)
        .setMessage(builder.toString())
        .setCancelable(false)
        .setNegativeButton(R.string.decline, null)
        .setPositiveButton(R.string.accept, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            Config.acceptRoutingDisclaimer();
            prepare(startPoint, endPoint, fromApi);
          }
        }).show();
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
  }

  public void deleteSavedRoute()
  {
    Framework.nativeDeleteSavedRoutePoints();
  }

  public void rebuildLastRoute()
  {
    setState(State.NONE);
    setBuildState(BuildState.NONE);
    prepare(getStartPoint(), getEndPoint(), false);
  }

  public void prepare(@Nullable MapObject startPoint, @Nullable MapObject endPoint)
  {
    prepare(startPoint, endPoint, false);
  }

  public void prepare(@Nullable MapObject startPoint, @Nullable MapObject endPoint, boolean fromApi)
  {
    Logger.d(TAG, "prepare (" + (endPoint == null ? "route)" : "p2p)"));

    if (!Config.isRoutingDisclaimerAccepted())
    {
      showDisclaimer(startPoint, endPoint, fromApi);
      return;
    }

    initLastRouteType(startPoint, endPoint, fromApi);
    prepare(startPoint, endPoint, mLastRouterType, fromApi);
  }

  private void initLastRouteType(@Nullable MapObject startPoint, @Nullable MapObject endPoint,
                                 boolean fromApi)
  {
    if (isSubwayEnabled() && !fromApi)
    {
      mLastRouterType = Framework.ROUTER_TYPE_TRANSIT;
      return;
    }

    if (startPoint != null && endPoint != null)
      mLastRouterType = Framework.nativeGetBestRouter(startPoint.getLat(), startPoint.getLon(),
                                                      endPoint.getLat(), endPoint.getLon());
  }

  private boolean isSubwayEnabled()
  {
    return mContainer != null && mContainer.isSubwayEnabled();
  }

  public void prepare(final @Nullable MapObject startPoint, final @Nullable MapObject endPoint,
                      @Framework.RouterType int routerType)
  {
    prepare(startPoint, endPoint, routerType, false);
  }

  public void prepare(final @Nullable MapObject startPoint, final @Nullable MapObject endPoint,
                      @Framework.RouterType int routerType, boolean fromApi)
  {
    cancel();
    setState(State.PREPARE);

    mLastRouterType = routerType;
    Framework.nativeSetRouter(mLastRouterType);

    if (startPoint != null || endPoint != null)
      setPointsInternal(startPoint, endPoint);

    startPlanning(startPoint, endPoint);
  }

  public void start()
  {
    Logger.d(TAG, "start");

    // This saving is needed just for situation when the user starts navigation
    // and then app crashes. So, the previous route will be restored on the next app launch.
    saveRoute();

    MapObject my = LocationHelper.INSTANCE.getMyPosition();

    if (my == null || !MapObject.isOfType(MapObject.MY_POSITION, getStartPoint()))
    {
      suggestRebuildRoute();
      return;
    }

    setState(State.NAVIGATION);

    cancelPlanning(false);
    startNavigation();

    Framework.nativeFollowRoute();
    LocationHelper.INSTANCE.restart();
  }

  public void addStop(@NonNull MapObject mapObject)
  {
    addRoutePoint(RoutePointInfo.ROUTE_MARK_INTERMEDIATE, mapObject);
    build();
    if (mContainer != null)
      mContainer.onAddedStop();
    resetToPlanningStateIfNavigating();
  }

  public void removeStop(@NonNull MapObject mapObject)
  {
    RoutePointInfo info = mapObject.getRoutePointInfo();
    if (info == null)
      throw new AssertionError("A stop point must have the route point info!");

    applyRemovingIntermediatePointsTransaction();
    Framework.nativeRemoveRoutePoint(info.mMarkType, info.mIntermediateIndex);
    build();
    if (mContainer != null)
      mContainer.onRemovedStop();
    resetToPlanningStateIfNavigating();
  }

  /**
   * @return False if not navigating, true otherwise
   */
  public boolean resetToPlanningStateIfNavigating()
  {
    if (isNavigating())
    {
      build();
      setState(State.PREPARE);
      cancelNavigation(false);
      startPlanning();
      if (mContainer != null)
        mContainer.updateMenu();
      if (mContainer != null)
        mContainer.onResetToPlanningState();
      return true;
    }
    return false;
  }

  private void removeIntermediatePoints()
  {
    Framework.nativeRemoveIntermediateRoutePoints();
  }

  @NonNull
  private MapObject toMapObject(@NonNull RouteMarkData point)
  {
    return MapObject.createMapObject(FeatureId.EMPTY, point.mIsMyPosition ? MapObject.MY_POSITION : MapObject.POI,
                         point.mTitle == null ? "" : point.mTitle,
                         point.mSubtitle == null ? "" : point.mSubtitle, point.mLat, point.mLon);
  }

  public boolean isStopPointAllowed()
  {
    return Framework.nativeCouldAddIntermediatePoint();
  }

  public boolean isRoutePoint(@NonNull MapObject mapObject)
  {
    return mapObject.getRoutePointInfo() != null;
  }

  private void suggestRebuildRoute()
  {
    if (mContainer == null)
      return;

    final AlertDialog.Builder builder = new AlertDialog.Builder(mContainer.requireActivity())
                                                       .setMessage(R.string.p2p_reroute_from_current)
                                                       .setCancelable(false)
                                                       .setNegativeButton(R.string.cancel, null);

    TextView titleView = (TextView)View.inflate(mContainer.requireActivity(), R.layout.dialog_suggest_reroute_title, null);
    titleView.setText(R.string.p2p_only_from_current);
    builder.setCustomTitle(titleView);

    if (MapObject.isOfType(MapObject.MY_POSITION, getEndPoint()))
    {
      builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
      {
        @Override
        public void onClick(DialogInterface dialog, int which)
        {
          swapPoints();
        }
      });
    }
    else
    {
      if (LocationHelper.INSTANCE.getMyPosition() == null)
        builder.setMessage(null).setNegativeButton(null, null);

      builder.setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
      {
        @Override
        public void onClick(DialogInterface dialog, int which)
        {
          setStartFromMyPosition();
        }
      });
    }

    builder.show();
  }

  private void updatePlan()
  {
    updateProgress();
  }

  private void cancelInternal()
  {
    Logger.d(TAG, "cancelInternal");

    //noinspection WrongConstant
    mWaitingPoiPickType = NO_WAITING_POI_PICK;

    setBuildState(BuildState.NONE);
    setState(State.NONE);

    applyRemovingIntermediatePointsTransaction();
    Framework.nativeDeleteSavedRoutePoints();
    Framework.nativeCloseRouting();
  }

  public boolean cancel()
  {
    if (isPlanning())
    {
      Logger.d(TAG, "cancel: planning");

      cancelInternal();
      cancelPlanning(true);
      return true;
    }

    if (isNavigating())
    {
      Logger.d(TAG, "cancel: navigating");

      cancelInternal();
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

  boolean isTransitType()
  {
    return mLastRouterType == Framework.ROUTER_TYPE_TRANSIT;
  }

  boolean isVehicleRouterType()
  {
    return mLastRouterType == Framework.ROUTER_TYPE_VEHICLE;
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

  public void waitForPoiPick(@RoutePointInfo.RouteMarkType int pointType){
    mWaitingPoiPickType = pointType;
  }

  public boolean isWaitingPoiPick()
  {
    return mWaitingPoiPickType != NO_WAITING_POI_PICK;
  }

  BuildState getBuildState()
  {
    return mBuildState;
  }

  @Nullable
  public MapObject getStartPoint()
  {
    return getStartOrEndPointByType(RoutePointInfo.ROUTE_MARK_START);
  }

  @Nullable
  public MapObject getEndPoint()
  {
    return getStartOrEndPointByType(RoutePointInfo.ROUTE_MARK_FINISH);
  }

  @Nullable
  private MapObject getStartOrEndPointByType(@RoutePointInfo.RouteMarkType int type)
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

    if (type == RoutePointInfo.ROUTE_MARK_START)
      return toMapObject(points[0]);
    if (type == RoutePointInfo.ROUTE_MARK_FINISH)
      return toMapObject(points[size - 1]);

    return null;
  }

  public boolean hasStartPoint()
  {
    return getStartPoint() != null;
  }

  public boolean hasEndPoint()
  {
    return getEndPoint() != null;
  }

  @Nullable
  RoutingInfo getCachedRoutingInfo()
  {
    return mCachedRoutingInfo;
  }

  @Nullable
  TransitRouteInfo getCachedTransitInfo()
  {
    return mCachedTransitRouteInfo;
  }

  private void setPointsInternal(@Nullable MapObject startPoint, @Nullable MapObject endPoint)
  {
    final boolean hasStart = startPoint != null;
    final boolean hasEnd = endPoint != null;
    final boolean hasOnePointAtLeast = hasStart || hasEnd;

    if (hasOnePointAtLeast)
      applyRemovingIntermediatePointsTransaction();

    if (hasStart)
      addRoutePoint(RoutePointInfo.ROUTE_MARK_START , startPoint);

    if (hasEnd)
      addRoutePoint(RoutePointInfo.ROUTE_MARK_FINISH , endPoint);

    if (hasOnePointAtLeast && mContainer != null)
      mContainer.updateMenu();
  }

  void checkAndBuildRoute()
  {
    if (isWaitingPoiPick())
      showRoutePlan();

    if (getStartPoint() != null && getEndPoint() != null)
      build();
  }

  private boolean setStartFromMyPosition()
  {
    Logger.d(TAG, "setStartFromMyPosition");

    MapObject my = LocationHelper.INSTANCE.getMyPosition();
    if (my == null)
    {
      Logger.d(TAG, "setStartFromMyPosition: no my position - skip");
      return false;
    }

    return setStartPoint(my);
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
    Logger.d(TAG, "setStartPoint");
    MapObject startPoint = getStartPoint();
    MapObject endPoint = getEndPoint();
    boolean isSamePoint = MapObject.same(startPoint, point);
    if (point != null)
    {
      applyRemovingIntermediatePointsTransaction();
      addRoutePoint(RoutePointInfo.ROUTE_MARK_START, point);
      startPoint = getStartPoint();
    }

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
   *   <li>Set starting point to MyPosition if it was not set before.
   * </ul>
   * Route starts to build if both points were set.
   *
   * @return {@code true} if the point was set.
   */
  @SuppressWarnings("Duplicates")
  public boolean setEndPoint(@Nullable MapObject point)
  {
    Logger.d(TAG, "setEndPoint");
    MapObject startPoint = getStartPoint();
    MapObject endPoint = getEndPoint();
    boolean isSamePoint = MapObject.same(endPoint, point);
    if (point != null)
    {
      applyRemovingIntermediatePointsTransaction();

      addRoutePoint(RoutePointInfo.ROUTE_MARK_FINISH, point);
      endPoint = getEndPoint();
    }

    if (isSamePoint)
    {
      Logger.d(TAG, "setEndPoint: skip the same end point");
      return false;
    }

    if (point != null && point.sameAs(startPoint))
    {
      if (endPoint == null)
      {
        Logger.d(TAG, "setEndPoint: skip because end point is empty");
        return false;
      }

      Logger.d(TAG, "setEndPoint: swap with starting point");
      startPoint = endPoint;

    }

    endPoint = point;
    setPointsInternal(startPoint, endPoint);
    checkAndBuildRoute();
    return true;
  }

  private static void addRoutePoint(@RoutePointInfo.RouteMarkType int type, @NonNull MapObject point)
  {
    Pair<String, String> description = getDescriptionForPoint(point);
    Framework.nativeAddRoutePoint(description.first /* title */, description.second /* subtitle */,
                                  type, 0 /* intermediateIndex */,
                                  MapObject.isOfType(MapObject.MY_POSITION, point),
                                  point.getLat(), point.getLon());
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
        title = Framework.nativeFormatLatLon(point.getLat(), point.getLon(), CoordinatesFormat.LatLonDecimal.getId());
      }
    }
    return new Pair<>(title, subtitle);
  }

  private void swapPoints()
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

  public void setRouterType(@Framework.RouterType int router)
  {
    Logger.d(TAG, "setRouterType: " + mLastRouterType + " -> " + router);

    // Repeating tap on Taxi icon should trigger the route building always,
    // because it may be "No internet connection, try later" case
    if (router == mLastRouterType)
      return;

    mLastRouterType = router;
    Framework.nativeSetRouter(router);

    cancelRemovingIntermediatePointsTransaction();

    if (getStartPoint() != null && getEndPoint() != null)
      build();
  }

  @Framework.RouterType
  public int getLastRouterType()
  {
    return mLastRouterType;
  }

  private void cancelRemovingIntermediatePointsTransaction()
  {
    if (mRemovingIntermediatePointsTransactionId == mInvalidRoutePointsTransactionId)
      return;
    Framework.nativeCancelRoutePointsTransaction(mRemovingIntermediatePointsTransactionId);
    mRemovingIntermediatePointsTransactionId = mInvalidRoutePointsTransactionId;
  }

  private void applyRemovingIntermediatePointsTransaction()
  {
    // We have to apply removing intermediate points transaction each time
    // we add/remove route points in the taxi mode.
    if (mRemovingIntermediatePointsTransactionId == mInvalidRoutePointsTransactionId)
      return;
    Framework.nativeApplyRoutePointsTransaction(mRemovingIntermediatePointsTransactionId);
    mRemovingIntermediatePointsTransactionId = mInvalidRoutePointsTransactionId;
  }

  public void onPoiSelected(@Nullable MapObject point)
  {
    if (!isWaitingPoiPick())
      return;

    if (mWaitingPoiPickType != RoutePointInfo.ROUTE_MARK_FINISH
        && mWaitingPoiPickType != RoutePointInfo.ROUTE_MARK_START)
    {
      throw new AssertionError("Only start and finish points can be added through search!");
    }

    if (point != null)
    {
      if (mWaitingPoiPickType == RoutePointInfo.ROUTE_MARK_FINISH)
        setEndPoint(point);
      else
        setStartPoint(point);
    }

    if (mContainer != null)
    {
      mContainer.updateMenu();
      showRoutePlan();
    }

    //noinspection WrongConstant
    mWaitingPoiPickType = NO_WAITING_POI_PICK;
  }
  public static CharSequence formatRoutingTime(Context context, int seconds, @DimenRes int unitsSize)
  {
    return formatRoutingTime(context, seconds, unitsSize, R.dimen.text_size_routing_number);
  }

  public static CharSequence formatRoutingTime(Context context, int seconds, @DimenRes int unitsSize,
                                               @DimenRes int textSize)
  {
    long minutes = TimeUnit.SECONDS.toMinutes(seconds) % 60;
    long hours = TimeUnit.SECONDS.toHours(seconds);
    String min = context.getString(R.string.minute);
    String hour = context.getString(R.string.hour);
    SpannableStringBuilder displayedH = Utils.formatUnitsText(context, textSize, unitsSize,
                                                              String.valueOf(hours), hour);
    SpannableStringBuilder displayedM = Utils.formatUnitsText(context, textSize, unitsSize,
                                                              String.valueOf(minutes), min);
    return hours == 0 ? displayedM : TextUtils.concat(displayedH + "\u00A0", displayedM);
  }

  static String formatArrivalTime(int seconds)
  {
    Calendar current = Calendar.getInstance();
    current.set(Calendar.SECOND, 0);
    current.add(Calendar.SECOND, seconds);
    return StringUtils.formatUsingUsLocale("%d:%02d", current.get(Calendar.HOUR_OF_DAY), current.get(Calendar.MINUTE));
  }
}
