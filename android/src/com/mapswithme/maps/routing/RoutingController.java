package com.mapswithme.maps.routing;

import android.app.Activity;
import android.content.Context;
import android.content.DialogInterface;
import android.support.annotation.DimenRes;
import android.support.annotation.IntRange;
import android.support.annotation.MainThread;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.FragmentActivity;
import android.support.v4.util.Pair;
import android.support.v7.app.AlertDialog;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.view.View;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.taxi.TaxiInfo;
import com.mapswithme.maps.taxi.TaxiInfoError;
import com.mapswithme.maps.taxi.TaxiManager;
import com.mapswithme.util.Config;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

import java.util.Calendar;
import java.util.concurrent.TimeUnit;

import static com.mapswithme.util.statistics.Statistics.EventName.ROUTING_POINT_ADD;
import static com.mapswithme.util.statistics.Statistics.EventName.ROUTING_POINT_REMOVE;

@android.support.annotation.UiThread
public class RoutingController implements TaxiManager.TaxiListener
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
    FragmentActivity getActivity();
    void showSearch();
    void showRoutePlan(boolean show, @Nullable Runnable completionListener);
    void showNavigation(boolean show);
    void showDownloader(boolean openDownloaded);
    void updateMenu();
    void onTaxiInfoReceived(@NonNull TaxiInfo info);
    void onTaxiError(@NonNull TaxiManager.ErrorCode code);
    void onNavigationCancelled();
    void onNavigationStarted();
    void onAddedStop();
    void onRemovedStop();
    void onBuiltRoute();

    /**
     * @param progress progress to be displayed.
     * */
    void updateBuildProgress(@IntRange(from = 0, to = 100) int progress, @Framework.RouterType int router);
  }

  private static final int NO_WAITING_POI_PICK = -1;
  private static final RoutingController sInstance = new RoutingController();
  private final Logger mLogger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.ROUTING);
  @Nullable
  private Container mContainer;

  private BuildState mBuildState = BuildState.NONE;
  private State mState = State.NONE;
  @RoutePointInfo.RouteMarkType
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
  private boolean mTaxiRequestHandled;
  private boolean mTaxiPlanning;
  private boolean mInternetConnected;

  private int mInvalidRoutePointsTransactionId;
  private int mRemovingIntermediatePointsTransactionId;

  @SuppressWarnings("FieldCanBeLocal")
  private final Framework.RoutingListener mRoutingListener = new Framework.RoutingListener()
  {
    @Override
    public void onRoutingEvent(final int resultCode, @Nullable final String[] missingMaps)
    {
      mLogger.d(TAG, "onRoutingEvent(resultCode: " + resultCode + ")");

      UiThread.run(new Runnable()
      {
        @Override
        public void run()
        {
          mLastResultCode = resultCode;
          mLastMissingMaps = missingMaps;
          mContainsCachedResult = true;

          if (mLastResultCode == ResultCodesHelper.NO_ERROR
              || ResultCodesHelper.isMoreMapsNeeded(mLastResultCode))
          {
            mCachedRoutingInfo = Framework.nativeGetRouteFollowingInfo();
            if (mLastRouterType == Framework.ROUTER_TYPE_TRANSIT)
              mCachedTransitRouteInfo = Framework.nativeGetTransitRouteInfo();
            setBuildState(BuildState.BUILT);
            mLastBuildProgress = 100;
            if (mContainer != null)
              mContainer.onBuiltRoute();
          }

          processRoutingEvent();
        }
      });
    }
  };

  @SuppressWarnings("FieldCanBeLocal")
  private final Framework.RoutingProgressListener mRoutingProgressListener = new Framework.RoutingProgressListener()
  {
    @Override
    public void onRouteBuildingProgress(final float progress)
    {
      UiThread.run(new Runnable()
      {
        @Override
        public void run()
        {
          mLastBuildProgress = (int) progress;
          updateProgress();
        }
      });
    }
  };

  @SuppressWarnings("FieldCanBeLocal")
  private final Framework.RoutingRecommendationListener mRoutingRecommendationListener =
      new Framework.RoutingRecommendationListener()
  {
    @Override
    public void onRecommend(@Framework.RouteRecommendationType final int recommendation)
    {
      UiThread.run(new Runnable()
      {
        @Override
        public void run()
        {
          if (recommendation == Framework.ROUTE_REBUILD_AFTER_POINTS_LOADING)
            setStartPoint(LocationHelper.INSTANCE.getMyPosition());
        }
      });
    }
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

    if (mLastResultCode == ResultCodesHelper.NO_ERROR)
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

    RoutingErrorDialogFragment fragment = RoutingErrorDialogFragment.create(mLastResultCode, mLastMissingMaps);
    fragment.show(mContainer.getActivity().getSupportFragmentManager(), RoutingErrorDialogFragment.class.getSimpleName());
  }

  private void setState(State newState)
  {
    mLogger.d(TAG, "[S] State: " + mState + " -> " + newState + ", BuildState: " + mBuildState);
    mState = newState;

    if (mContainer != null)
      mContainer.updateMenu();
  }

  private void setBuildState(BuildState newState)
  {
    mLogger.d(TAG, "[B] State: " + mState + ", BuildState: " + mBuildState + " -> " + newState);
    mBuildState = newState;

    if (mBuildState == BuildState.BUILT && !MapObject.isOfType(MapObject.MY_POSITION, getStartPoint()))
      Framework.nativeDisableFollowing();

    if (mContainer != null)
      mContainer.updateMenu();
  }

  private void updateProgress()
  {
    if (isTaxiPlanning())
      return;

    if (mContainer != null)
      mContainer.updateBuildProgress(mLastBuildProgress, mLastRouterType);
  }

  private void showRoutePlan()
  {
    if (mContainer != null)
      mContainer.showRoutePlan(true, new Runnable()
      {
        @Override
        public void run()
        {
          updatePlan();
        }
      });
  }

  public void attach(@NonNull Container container)
  {
    mContainer = container;
  }

  public void initialize()
  {
    mLastRouterType = Framework.nativeGetLastUsedRouter();
    mInvalidRoutePointsTransactionId = Framework.nativeInvalidRoutePointsTransactionId();
    mRemovingIntermediatePointsTransactionId = mInvalidRoutePointsTransactionId;

    Framework.nativeSetRoutingListener(mRoutingListener);
    Framework.nativeSetRouteProgressListener(mRoutingProgressListener);
    Framework.nativeSetRoutingRecommendationListener(mRoutingRecommendationListener);
    TaxiManager.INSTANCE.setTaxiListener(this);
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
      if (isTaxiPlanning())
        mContainer.updateBuildProgress(0, mLastRouterType);

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

    mLogger.d(TAG, "build");
    mTaxiRequestHandled = false;
    mLastBuildProgress = 0;
    mInternetConnected = ConnectionState.isConnected();

    if (isTaxiRouterType())
    {
      if (!mInternetConnected)
      {
        completeTaxiRequest();
        return;
      }

      MapObject start = getStartPoint();
      MapObject end = getEndPoint();
      if (start != null && end != null)
        requestTaxiInfo(start, end);
    }

    setBuildState(BuildState.BUILDING);
    updatePlan();

    Statistics.INSTANCE.trackRouteBuild(mLastRouterType, getStartPoint(), getEndPoint());
    org.alohalytics.Statistics.logEvent(AlohaHelper.ROUTING_BUILD,
            new String[]{Statistics.EventParam.FROM, Statistics.getPointType(getStartPoint()),
                         Statistics.EventParam.TO, Statistics.getPointType(getEndPoint())});

    Framework.nativeBuildRoute();
  }

  private void completeTaxiRequest()
  {
    mTaxiRequestHandled = true;
    if (mContainer != null)
    {
      mContainer.updateBuildProgress(100, mLastRouterType);
      mContainer.updateMenu();
    }
  }

  private void showDisclaimer(final MapObject startPoint, final MapObject endPoint,
                              final boolean fromApi)
  {
    if (mContainer == null)
      return;

    StringBuilder builder = new StringBuilder();
    for (int resId : new int[] { R.string.dialog_routing_disclaimer_priority, R.string.dialog_routing_disclaimer_precision,
                                 R.string.dialog_routing_disclaimer_recommendations, R.string.dialog_routing_disclaimer_borders,
                                 R.string.dialog_routing_disclaimer_beware })
      builder.append(MwmApplication.get().getString(resId)).append("\n\n");

    new AlertDialog.Builder(mContainer.getActivity())
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
    if (Framework.nativeHasSavedRoutePoints())
    {
      Framework.nativeLoadRoutePoints();
      prepare(getStartPoint(), getEndPoint());
    }
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

  public void prepare(boolean canUseMyPositionAsStart, @Nullable MapObject endPoint)
  {
    prepare(canUseMyPositionAsStart, endPoint, false);
  }

  public void prepare(boolean canUseMyPositionAsStart, @Nullable MapObject endPoint, boolean fromApi)
  {
    MapObject startPoint = canUseMyPositionAsStart ? LocationHelper.INSTANCE.getMyPosition() : null;
    prepare(startPoint, endPoint, fromApi);
  }

  public void prepare(boolean canUseMyPositionAsStart, @Nullable MapObject endPoint,
                      @Framework.RouterType int type, boolean fromApi)
  {
    MapObject startPoint = canUseMyPositionAsStart ? LocationHelper.INSTANCE.getMyPosition() : null;
    prepare(startPoint, endPoint, type, fromApi);
  }

  public void prepare(@Nullable MapObject startPoint, @Nullable MapObject endPoint)
  {
    prepare(startPoint, endPoint, false);
  }

  public void prepare(@Nullable MapObject startPoint, @Nullable MapObject endPoint, boolean fromApi)
  {
    mLogger.d(TAG, "prepare (" + (endPoint == null ? "route)" : "p2p)"));

    if (!Config.isRoutingDisclaimerAccepted())
    {
      showDisclaimer(startPoint, endPoint, fromApi);
      return;
    }

    if (startPoint != null && endPoint != null)
      mLastRouterType = Framework.nativeGetBestRouter(startPoint.getLat(), startPoint.getLon(),
                                                      endPoint.getLat(), endPoint.getLon());
    prepare(startPoint, endPoint, mLastRouterType, fromApi);
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

    if (mContainer != null)
      mContainer.showRoutePlan(true, new Runnable()
      {
        @Override
        public void run()
        {
          if (startPoint == null || endPoint == null)
            updatePlan();
          else
            build();
        }
      });

    if (startPoint != null)
      trackPointAdd(startPoint, RoutePointInfo.ROUTE_MARK_START, false, false, fromApi);
    if (endPoint != null)
      trackPointAdd(endPoint, RoutePointInfo.ROUTE_MARK_FINISH, false, false, fromApi);
  }

  private static void trackPointAdd(@NonNull MapObject point, @RoutePointInfo.RouteMarkType int type,
                          boolean isPlanning, boolean isNavigating, boolean fromApi)
  {
    boolean isMyPosition = point.getMapObjectType() == MapObject.MY_POSITION;
    Statistics.INSTANCE.trackRoutingPoint(ROUTING_POINT_ADD, type, isPlanning, isNavigating,
                                          isMyPosition, fromApi);
  }

  private static void trackPointRemove(@NonNull MapObject point, @RoutePointInfo.RouteMarkType int type,
                             boolean isPlanning, boolean isNavigating, boolean fromApi)
  {
    boolean isMyPosition = point.getMapObjectType() == MapObject.MY_POSITION;
    Statistics.INSTANCE.trackRoutingPoint(ROUTING_POINT_REMOVE, type, isPlanning, isNavigating,
                                          isMyPosition, fromApi);
  }

  public void start()
  {
    mLogger.d(TAG, "start");

    // This saving is needed just for situation when the user starts navigation
    // and then app crashes. So, the previous route will be restored on the next app launch.
    saveRoute();

    MapObject my = LocationHelper.INSTANCE.getMyPosition();

    if (my == null || !MapObject.isOfType(MapObject.MY_POSITION, getStartPoint()))
    {
      Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_START_SUGGEST_REBUILD);
      AlohaHelper.logClick(AlohaHelper.ROUTING_START_SUGGEST_REBUILD);
      suggestRebuildRoute();
      return;
    }

    Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_START);
    AlohaHelper.logClick(AlohaHelper.ROUTING_START);
    setState(State.NAVIGATION);

    if (mContainer != null)
    {
      mContainer.showRoutePlan(false, null);
      mContainer.showNavigation(true);
      mContainer.onNavigationStarted();
    }

    Framework.nativeFollowRoute();
    LocationHelper.INSTANCE.restart();
  }

  public void addStop(@NonNull MapObject mapObject)
  {
    addRoutePoint(RoutePointInfo.ROUTE_MARK_INTERMEDIATE, mapObject);
    trackPointAdd(mapObject, RoutePointInfo.ROUTE_MARK_INTERMEDIATE, isPlanning(), isNavigating(),
                  false);
    build();
    if (mContainer != null)
      mContainer.onAddedStop();
    backToPlaningStateIfNavigating();
  }

  public void removeStop(@NonNull MapObject mapObject)
  {
    RoutePointInfo info = mapObject.getRoutePointInfo();
    if (info == null)
      throw new AssertionError("A stop point must have the route point info!");

    applyRemovingIntermediatePointsTransaction();
    Framework.nativeRemoveRoutePoint(info.mMarkType, info.mIntermediateIndex);
    trackPointRemove(mapObject, info.mMarkType, isPlanning(), isNavigating(), false);
    build();
    if (mContainer != null)
      mContainer.onRemovedStop();
    backToPlaningStateIfNavigating();
  }

  private void backToPlaningStateIfNavigating()
  {
    if (!isNavigating())
      return;

    setState(State.PREPARE);
    if (mContainer != null)
    {
      mContainer.showNavigation(false);
      mContainer.showRoutePlan(true, null);
      mContainer.updateMenu();
      mContainer.onNavigationCancelled();
    }
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
    return Framework.nativeCouldAddIntermediatePoint() && !isTaxiRouterType();
  }

  public boolean isRoutePoint(@NonNull MapObject mapObject)
  {
    return mapObject.getRoutePointInfo() != null;
  }

  private void suggestRebuildRoute()
  {
    if (mContainer == null)
      return;

    final AlertDialog.Builder builder = new AlertDialog.Builder(mContainer.getActivity())
                                                       .setMessage(R.string.p2p_reroute_from_current)
                                                       .setCancelable(false)
                                                       .setNegativeButton(R.string.cancel, null);

    TextView titleView = (TextView)View.inflate(mContainer.getActivity(), R.layout.dialog_suggest_reroute_title, null);
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
    mLogger.d(TAG, "cancelInternal");

    //noinspection WrongConstant
    mWaitingPoiPickType = NO_WAITING_POI_PICK;
    mTaxiRequestHandled = false;

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
      mLogger.d(TAG, "cancel: planning");

      cancelInternal();
      if (mContainer != null)
        mContainer.showRoutePlan(false, null);
      return true;
    }

    if (isNavigating())
    {
      mLogger.d(TAG, "cancel: navigating");

      cancelInternal();
      if (mContainer != null)
      {
        mContainer.showNavigation(false);
        mContainer.updateMenu();
      }
      if (mContainer != null)
        mContainer.onNavigationCancelled();
      return true;
    }

    mLogger.d(TAG, "cancel: none");
    return false;
  }

  public boolean isPlanning()
  {
    return mState == State.PREPARE;
  }

  boolean isTaxiPlanning()
  {
    return isTaxiRouterType() && mTaxiPlanning;
  }

  boolean isTaxiRouterType()
  {
    return mLastRouterType == Framework.ROUTER_TYPE_TAXI;
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

  public boolean isTaxiRequestHandled()
  {
    return mTaxiRequestHandled;
  }

  boolean isInternetConnected()
  {
    return mInternetConnected;
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
    if (startPoint != null)
    {
      applyRemovingIntermediatePointsTransaction();
      addRoutePoint(RoutePointInfo.ROUTE_MARK_START, startPoint);
      if (mContainer != null)
        mContainer.updateMenu();
    }

    if (endPoint != null)
    {
      applyRemovingIntermediatePointsTransaction();
      addRoutePoint(RoutePointInfo.ROUTE_MARK_FINISH, endPoint);
      if (mContainer != null)
        mContainer.updateMenu();
    }
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
    mLogger.d(TAG, "setStartFromMyPosition");

    MapObject my = LocationHelper.INSTANCE.getMyPosition();
    if (my == null)
    {
      mLogger.d(TAG, "setStartFromMyPosition: no my position - skip");
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
    mLogger.d(TAG, "setStartPoint");
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
      mLogger.d(TAG, "setStartPoint: skip the same starting point");
      return false;
    }

    if (point != null && point.sameAs(endPoint))
    {
      if (startPoint == null)
      {
        mLogger.d(TAG, "setStartPoint: skip because starting point is empty");
        return false;
      }

      mLogger.d(TAG, "setStartPoint: swap with end point");
      endPoint = startPoint;
    }

    startPoint = point;
    setPointsInternal(startPoint, endPoint);
    checkAndBuildRoute();
    if (startPoint != null)
      trackPointAdd(startPoint, RoutePointInfo.ROUTE_MARK_START, isPlanning(), isNavigating(),
                    false);
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
    mLogger.d(TAG, "setEndPoint");
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
      mLogger.d(TAG, "setEndPoint: skip the same end point");
      return false;
    }

    if (point != null && point.sameAs(startPoint))
    {
      if (endPoint == null)
      {
        mLogger.d(TAG, "setEndPoint: skip because end point is empty");
        return false;
      }

      mLogger.d(TAG, "setEndPoint: swap with starting point");
      startPoint = endPoint;

    }

    endPoint = point;

    if (endPoint != null)
      trackPointAdd(endPoint, RoutePointInfo.ROUTE_MARK_FINISH, isPlanning(), isNavigating(),
                    false);

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
        title = Framework.nativeFormatLatLon(point.getLat(), point.getLon(), false /* useDmsFormat */);
      }
    }
    return new Pair<>(title, subtitle);
  }

  private void swapPoints()
  {
    mLogger.d(TAG, "swapPoints");

    MapObject startPoint = getStartPoint();
    MapObject endPoint = getEndPoint();
    MapObject point = startPoint;
    startPoint = endPoint;
    endPoint = point;

    Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_SWAP_POINTS);
    AlohaHelper.logClick(AlohaHelper.ROUTING_SWAP_POINTS);

    setPointsInternal(startPoint, endPoint);
    checkAndBuildRoute();
    if (mContainer != null)
      mContainer.updateMenu();
  }

  public void setRouterType(@Framework.RouterType int router)
  {
    mLogger.d(TAG, "setRouterType: " + mLastRouterType + " -> " + router);

    // Repeating tap on Taxi icon should trigger the route building always,
    // because it may be "No internet connection, try later" case
    if (router == mLastRouterType && !isTaxiRouterType())
      return;

    mLastRouterType = router;
    Framework.nativeSetRouter(router);

    // Taxi routing does not support intermediate points.
    if (isTaxiRouterType())
    {
      openRemovingIntermediatePointsTransaction();
      removeIntermediatePoints();
    }
    else
    {
      cancelRemovingIntermediatePointsTransaction();
    }

    if (getStartPoint() != null && getEndPoint() != null)
      build();
  }

  private void openRemovingIntermediatePointsTransaction()
  {
    if (mRemovingIntermediatePointsTransactionId == mInvalidRoutePointsTransactionId)
      mRemovingIntermediatePointsTransactionId = Framework.nativeOpenRoutePointsTransaction();
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
    long minutes = TimeUnit.SECONDS.toMinutes(seconds) % 60;
    long hours = TimeUnit.SECONDS.toHours(seconds);
    String min = context.getString(R.string.minute);
    String hour = context.getString(R.string.hour);
    @DimenRes
    int textSize = R.dimen.text_size_routing_number;
    SpannableStringBuilder displayedH = Utils.formatUnitsText(context, textSize, unitsSize,
                                                              String.valueOf(hours), hour);
    SpannableStringBuilder displayedM = Utils.formatUnitsText(context, textSize, unitsSize,
                                                              String.valueOf(minutes), min);
    return hours == 0 ? displayedM : TextUtils.concat(displayedH + " ", displayedM);
  }

  static String formatArrivalTime(int seconds)
  {
    Calendar current = Calendar.getInstance();
    current.set(Calendar.SECOND, 0);
    current.add(Calendar.SECOND, seconds);
    return StringUtils.formatUsingUsLocale("%d:%02d", current.get(Calendar.HOUR_OF_DAY), current.get(Calendar.MINUTE));
  }

  public boolean checkMigration(Activity activity)
  {
    if (!MapManager.nativeIsLegacyMode())
      return false;

    if (!isNavigating() && !isPlanning())
      return false;

    new AlertDialog.Builder(activity)
        .setTitle(R.string.migrate_title)
        .setMessage(R.string.no_migration_during_navigation)
        .setPositiveButton(android.R.string.ok, null)
        .show();

    return true;
  }

  private void requestTaxiInfo(@NonNull MapObject startPoint, @NonNull MapObject endPoint)
  {
    mTaxiPlanning = true;

    TaxiManager.INSTANCE.nativeRequestTaxiProducts(NetworkPolicy.newInstance(true /* canUse */),
                                   startPoint.getLat(), startPoint.getLon(),
                                   endPoint.getLat(), endPoint.getLon());
    if (mContainer != null)
      mContainer.updateBuildProgress(0, mLastRouterType);
  }

  @Override
  public void onTaxiProviderReceived(@NonNull TaxiInfo provider)
  {
    mTaxiPlanning = false;
    mLogger.d(TAG, "onTaxiInfoReceived provider = " + provider);
    if (isTaxiRouterType() && mContainer != null)
    {
      mContainer.onTaxiInfoReceived(provider);
      completeTaxiRequest();
      Statistics.INSTANCE.trackTaxiEvent(Statistics.EventName.ROUTING_TAXI_ROUTE_BUILT,
                                         provider.getType());
    }
  }

  @Override
  public void onTaxiErrorReceived(@NonNull TaxiInfoError error)
  {
    mTaxiPlanning = false;
    mLogger.e(TAG, "onTaxiError error = " + error);
    if (isTaxiRouterType() && mContainer != null)
    {
      mContainer.onTaxiError(error.getCode());
      completeTaxiRequest();
      Statistics.INSTANCE.trackTaxiError(error);
    }
  }

  @Override
  public void onNoTaxiProviders()
  {
    mTaxiPlanning = false;
    mLogger.e(TAG, "onNoTaxiProviders");
    if (isTaxiRouterType() && mContainer != null)
    {
      mContainer.onTaxiError(TaxiManager.ErrorCode.NoProviders);
      completeTaxiRequest();
      Statistics.INSTANCE.trackNoTaxiProvidersError();
    }
  }
}
