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
import android.support.v4.app.FragmentManager;
import android.support.v7.app.AlertDialog;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.view.View;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.uber.Uber;
import com.mapswithme.maps.uber.UberInfo;
import com.mapswithme.maps.uber.UberLinks;
import com.mapswithme.util.Config;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.ThemeSwitcher;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

import java.util.Calendar;
import java.util.concurrent.TimeUnit;

@android.support.annotation.UiThread
public class RoutingController
{
  private static final String TAG = RoutingController.class.getSimpleName();
  private static final int NO_SLOT = 0;

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
    void updatePoints();
    void onUberInfoReceived(@NonNull UberInfo info);
    void onUberError(@NonNull Uber.ErrorCode code);
    void onNavigationCancelled();

    /**
     * @param progress progress to be displayed.
     * */
    void updateBuildProgress(@IntRange(from = 0, to = 100) int progress, @Framework.RouterType int router);
  }

  private static final RoutingController sInstance = new RoutingController();
  private final Logger mLogger = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.ROUTING);
  @Nullable
  private Container mContainer;

  private BuildState mBuildState = BuildState.NONE;
  private State mState = State.NONE;
  private int mWaitingPoiPickSlot = NO_SLOT;

  @Nullable
  private MapObject mStartPoint;
  @Nullable
  private MapObject mEndPoint;

  private int mLastBuildProgress;
  @Framework.RouterType
  private int mLastRouterType = Framework.nativeGetLastUsedRouter();

  private boolean mHasContainerSavedState;
  private boolean mContainsCachedResult;
  private int mLastResultCode;
  private String[] mLastMissingMaps;
  @Nullable
  private RoutingInfo mCachedRoutingInfo;
  private boolean mUberRequestHandled;
  private boolean mUberPlanning;
  private boolean mInternetConnected;

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
            setBuildState(BuildState.BUILT);
            mLastBuildProgress = 100;
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

    if (mBuildState == BuildState.BUILT && !MapObject.isOfType(MapObject.MY_POSITION, mStartPoint))
      Framework.nativeDisableFollowing();

    if (mContainer != null)
      mContainer.updateMenu();
  }

  private void updateProgress()
  {
    if (isUberPlanning())
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
    Framework.nativeSetRoutingListener(mRoutingListener);
    Framework.nativeSetRouteProgressListener(mRoutingProgressListener);
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
      if (isUberPlanning())
        mContainer.updateBuildProgress(0, mLastRouterType);

      mContainer.showNavigation(isNavigating());
      mContainer.updateMenu();
      mContainer.updatePoints();
    }
    processRoutingEvent();
  }

  public void onSaveState()
  {
    mHasContainerSavedState = true;
  }

  private void build()
  {
    mLogger.d(TAG, "build");
    mUberRequestHandled = false;
    mLastBuildProgress = 0;
    mInternetConnected = ConnectionState.isConnected();

    if (isTaxiRouterType())
    {
      if (!mInternetConnected)
      {
        completeUberRequest();
        return;
      }
      if (mContainer != null)
        requestUberInfo(mContainer.getActivity().getSupportFragmentManager());
    }

    setBuildState(BuildState.BUILDING);
    updatePlan();

    boolean isP2P = !MapObject.isOfType(MapObject.MY_POSITION, mStartPoint) && !MapObject.isOfType(MapObject.MY_POSITION, mEndPoint);

    Statistics.INSTANCE.trackRouteBuild(mLastRouterType, mStartPoint, mEndPoint);
    org.alohalytics.Statistics.logEvent(AlohaHelper.ROUTING_BUILD, new String[]{Statistics.EventParam.FROM, Statistics.getPointType(mStartPoint),
                                                                                Statistics.EventParam.TO, Statistics.getPointType(mEndPoint)});
    Framework.nativeBuildRoute(mStartPoint.getLat(), mStartPoint.getLon(), mEndPoint.getLat(), mEndPoint.getLon(), isP2P);
  }

  private void completeUberRequest()
  {
    mUberRequestHandled = true;
    if (mContainer != null)
    {
      mContainer.updateBuildProgress(100, mLastRouterType);
      mContainer.updateMenu();
    }
  }

  private void showDisclaimer(final MapObject startPoint, final MapObject endPoint)
  {
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
            prepare(startPoint, endPoint);
          }
        }).show();
  }

  public void prepare(@Nullable MapObject endPoint)
  {
    prepare(LocationHelper.INSTANCE.getMyPosition(), endPoint);
  }

  public void prepare(@Nullable MapObject startPoint, @Nullable MapObject endPoint)
  {
    mLogger.d(TAG, "prepare (" + (endPoint == null ? "route)" : "p2p)"));

    if (!Config.isRoutingDisclaimerAccepted())
    {
      showDisclaimer(startPoint, endPoint);
      return;
    }

    if (startPoint != null && endPoint != null)
      mLastRouterType = Framework.nativeGetBestRouter(startPoint.getLat(), startPoint.getLon(),
                                                      endPoint.getLat(), endPoint.getLon());
    prepare(startPoint, endPoint, mLastRouterType);
  }

  public void prepare(@Nullable MapObject startPoint, @Nullable MapObject endPoint,
                      @Framework.RouterType int routerType)
  {
    cancel();
    mStartPoint = startPoint;
    mEndPoint = endPoint;
    setState(State.PREPARE);

    mLastRouterType = routerType;
    Framework.nativeSetRouter(mLastRouterType);

    if (mStartPoint != null || mEndPoint != null)
      setPointsInternal();

    if (mContainer != null)
      mContainer.showRoutePlan(true, new Runnable()
      {
        @Override
        public void run()
        {
          if (mStartPoint == null || mEndPoint == null)
            updatePlan();
          else
            build();
        }
      });
  }

  public void start()
  {
    mLogger.d(TAG, "start");

    if (!MapObject.isOfType(MapObject.MY_POSITION, mStartPoint))
    {
      Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_START_SUGGEST_REBUILD);
      AlohaHelper.logClick(AlohaHelper.ROUTING_START_SUGGEST_REBUILD);
      suggestRebuildRoute();
      return;
    }

    MapObject my = LocationHelper.INSTANCE.getMyPosition();
    if (my == null)
    {
      mRoutingListener.onRoutingEvent(ResultCodesHelper.NO_POSITION, null);
      return;
    }

    mStartPoint = my;
    Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_START);
    AlohaHelper.logClick(AlohaHelper.ROUTING_START);
    setState(State.NAVIGATION);

    mContainer.showRoutePlan(false, null);
    mContainer.showNavigation(true);

    ThemeSwitcher.restart();

    Framework.nativeFollowRoute();
    LocationHelper.INSTANCE.restart();
  }

  private void suggestRebuildRoute()
  {
    final AlertDialog.Builder builder = new AlertDialog.Builder(mContainer.getActivity())
                                                       .setMessage(R.string.p2p_reroute_from_current)
                                                       .setCancelable(false)
                                                       .setNegativeButton(R.string.cancel, null);

    TextView titleView = (TextView)View.inflate(mContainer.getActivity(), R.layout.dialog_suggest_reroute_title, null);
    titleView.setText(R.string.p2p_only_from_current);
    builder.setCustomTitle(titleView);

    if (MapObject.isOfType(MapObject.MY_POSITION, mEndPoint))
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

    mStartPoint = null;
    mEndPoint = null;
    setPointsInternal();
    mWaitingPoiPickSlot = NO_SLOT;
    mUberRequestHandled = false;

    setBuildState(BuildState.NONE);
    setState(State.NONE);

    ThemeSwitcher.restart();
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

  boolean isUberPlanning()
  {
    return isTaxiRouterType() && mUberPlanning;
  }

  boolean isTaxiRouterType()
  {
    return mLastRouterType == Framework.ROUTER_TYPE_TAXI;
  }

  boolean isVehicleRouterType()
  {
    return mLastRouterType == Framework.ROUTER_TYPE_VEHICLE;
  }

  public boolean isNavigating()
  {
    return mState == State.NAVIGATION;
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


  public boolean isWaitingPoiPick()
  {
    return (mWaitingPoiPickSlot != NO_SLOT);
  }

  public boolean isUberRequestHandled()
  {
    return mUberRequestHandled;
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
  MapObject getStartPoint()
  {
    return mStartPoint;
  }

  @Nullable
  MapObject getEndPoint()
  {
    return mEndPoint;
  }

  @Nullable
  RoutingInfo getCachedRoutingInfo()
  {
    return mCachedRoutingInfo;
  }

  private void setPointsInternal()
  {
    if (mStartPoint == null)
      Framework.nativeSetRouteStartPoint(0.0, 0.0, false);
    else
      Framework.nativeSetRouteStartPoint(mStartPoint.getLat(), mStartPoint.getLon(),
                                         !MapObject.isOfType(MapObject.MY_POSITION, mStartPoint));

    if (mEndPoint == null)
      Framework.nativeSetRouteEndPoint(0.0, 0.0, false);
    else
      Framework.nativeSetRouteEndPoint(mEndPoint.getLat(), mEndPoint.getLon(), true);
  }

  void checkAndBuildRoute()
  {
    if (mContainer != null)
    {
      if (isWaitingPoiPick())
        showRoutePlan();

      mContainer.updatePoints();
    }

    if (mStartPoint != null && mEndPoint != null)
      build();
  }

  private boolean setStartFromMyPosition()
  {
    mLogger.d(TAG, "setStartFromMyPosition");

    MapObject my = LocationHelper.INSTANCE.getMyPosition();
    if (my == null)
    {
      mLogger.d(TAG, "setStartFromMyPosition: no my position - skip");

      if (mContainer != null)
        mContainer.updatePoints();

      setPointsInternal();
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
  public boolean setStartPoint(MapObject point)
  {
    mLogger.d(TAG, "setStartPoint");

    if (MapObject.same(mStartPoint, point))
    {
      mLogger.d(TAG, "setStartPoint: skip the same starting point");
      return false;
    }

    if (point != null && point.sameAs(mEndPoint))
    {
      if (mStartPoint == null)
      {
        mLogger.d(TAG, "setStartPoint: skip because starting point is empty");
        return false;
      }

      mLogger.d(TAG, "setStartPoint: swap with end point");
      mEndPoint = mStartPoint;
    }

    mStartPoint = point;
    setPointsInternal();
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
  public boolean setEndPoint(MapObject point)
  {
    mLogger.d(TAG, "setEndPoint");

    if (MapObject.same(mEndPoint, point))
    {
      if (mStartPoint == null)
        return setStartFromMyPosition();

      mLogger.d(TAG, "setEndPoint: skip the same end point");
      return false;
    }

    if (point != null && point.sameAs(mStartPoint))
    {
      if (mEndPoint == null)
      {
        mLogger.d(TAG, "setEndPoint: skip because end point is empty");
        return false;
      }

      mLogger.d(TAG, "setEndPoint: swap with starting point");
      mStartPoint = mEndPoint;
    }

    mEndPoint = point;

    if (mStartPoint == null)
      return setStartFromMyPosition();

    setPointsInternal();
    checkAndBuildRoute();
    return true;
  }

  void swapPoints()
  {
    mLogger.d(TAG, "swapPoints");

    MapObject point = mStartPoint;
    mStartPoint = mEndPoint;
    mEndPoint = point;

    Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_SWAP_POINTS);
    AlohaHelper.logClick(AlohaHelper.ROUTING_SWAP_POINTS);

    setPointsInternal();
    checkAndBuildRoute();
  }

  public void setRouterType(@Framework.RouterType int router)
  {
    mLogger.d(TAG, "setRouterType: " + mLastRouterType + " -> " + router);

    // Repeating tap on Uber icon should trigger the route building always,
    // because it may be "No internet connection, try later" case
    if (router == mLastRouterType && !isTaxiRouterType())
      return;

    mLastRouterType = router;
    Framework.nativeSetRouter(router);

    if (mStartPoint != null && mEndPoint != null)
      build();
  }

  void searchPoi(int slotId)
  {
    mLogger.d(TAG, "searchPoi: " + slotId);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.ROUTING_SEARCH_POINT);
    AlohaHelper.logClick(AlohaHelper.ROUTING_SEARCH_POINT);
    mWaitingPoiPickSlot = slotId;
    if (mContainer != null)
    {
      mContainer.showSearch();
      mContainer.updateMenu();
    }
  }

  private void onPoiSelectedInternal(@Nullable MapObject point, int slot)
  {
    if (point != null)
    {
      if (slot == 1)
        setStartPoint(point);
      else
        setEndPoint(point);
    }

    if (mContainer == null)
      return;

    mContainer.updateMenu();
    showRoutePlan();
  }

  public void onPoiSelected(@Nullable MapObject point)
  {
    int slot = mWaitingPoiPickSlot;
    mWaitingPoiPickSlot = NO_SLOT;

    onPoiSelectedInternal(point, slot);
    if (mContainer != null)
      mContainer.updatePoints();
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

  private void requestUberInfo(@NonNull FragmentManager fragmentManager)
  {
    NetworkPolicy.checkNetworkPolicy(fragmentManager, new NetworkPolicy.NetworkPolicyListener()
    {
      @Override
      public void onResult(@NonNull NetworkPolicy policy)
      {
        mUberPlanning = true;
        Uber.nativeRequestUberProducts(policy, mStartPoint.getLat(),
                                       mStartPoint.getLon(),
                                       mEndPoint.getLat(), mEndPoint.getLon());
        if (mContainer != null)
          mContainer.updateBuildProgress(0, mLastRouterType);
      }
    });
  }

  @NonNull
  UberLinks getUberLink(@NonNull String productId)
  {
    return Uber.nativeGetUberLinks(productId, mStartPoint.getLat(), mStartPoint.getLon(),
                                   mEndPoint.getLat(), mEndPoint.getLon());
  }

  /**
   * Called from the native code
   * @param info this object contains information about Uber products
   */
  @MainThread
  private void onUberInfoReceived(@NonNull UberInfo info)
  {
    mUberPlanning = false;
    mLogger.d(TAG, "onUberInfoReceived uberInfo = " + info);
    if (isTaxiRouterType() && mContainer != null)
    {
      mContainer.onUberInfoReceived(info);
      completeUberRequest();
    }
  }

  /**
   * Called from the native code
   * @param errorCode must match the one of the values in {@link com.mapswithme.maps.uber.Uber.ErrorCode}
   */
  @MainThread
  private void onUberError(@NonNull String errorCode)
  {
    mUberPlanning = false;
    Uber.ErrorCode code = Uber.ErrorCode.valueOf(errorCode);
    mLogger.e(TAG, "onUberError error = " + code);
    if (isTaxiRouterType() && mContainer != null)
    {
      mContainer.onUberError(code);
      completeUberRequest();
    }
  }
}
