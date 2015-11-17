package com.mapswithme.maps.routing;

import android.content.DialogInterface;
import android.support.annotation.IntRange;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.FragmentActivity;
import android.support.v7.app.AlertDialog;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import com.mapswithme.country.ActiveCountryTree;
import com.mapswithme.country.StorageOptions;
import com.mapswithme.maps.*;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.Config;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;

import java.util.Calendar;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

@android.support.annotation.UiThread
public class RoutingController
{
  public static final int NO_SLOT = 0;

  private static final String TAG = "RCSTATE";

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
    void showDownloader(boolean openDownloadedList);
    void updateMenu();
    void updatePoints();

    /**
     * @param progress progress to be displayed.
     * @param router selected router type. One of {@link Framework#ROUTER_TYPE_VEHICLE} and {@link Framework#ROUTER_TYPE_PEDESTRIAN}.
     * */
    void updateBuildProgress(@IntRange(from = 0, to = 100) int progress, int router);
  }

  private static final RoutingController sInstance = new RoutingController();

  private Container mContainer;
  private View mStartButton;

  private BuildState mBuildState = BuildState.NONE;
  private State mState = State.NONE;
  private int mWaitingPoiPickSlot = NO_SLOT;

  private MapObject mStartPoint;
  private MapObject mEndPoint;

  private int mLastBuildProgress;
  private int mLastRouterType = Framework.nativeGetLastUsedRouter();

  @SuppressWarnings("FieldCanBeLocal")
  private final Framework.RoutingListener mRoutingListener = new Framework.RoutingListener()
  {
    @Override
    public void onRoutingEvent(final int resultCode, final MapStorage.Index[] missingCountries, final MapStorage.Index[] missingRoutes)
    {
      Log.d(TAG, "onRoutingEvent(resultCode: " + resultCode + ")");

      UiThread.run(new Runnable()
      {
        @Override
        public void run()
        {
          if (resultCode == ResultCodesHelper.NO_ERROR)
          {
            setBuildState(BuildState.BUILT);
            mLastBuildProgress = 100;
            updatePlan();
            return;
          }

          if (mContainer == null)
            return;

          setBuildState(BuildState.ERROR);
          mLastBuildProgress = 0;
          updateProgress();

          RoutingErrorDialogFragment fragment = RoutingErrorDialogFragment.create(resultCode, missingCountries, missingRoutes);
          fragment.setListener(new RoutingErrorDialogFragment.Listener()
          {
            @Override
            public void onDownload()
            {
              cancel();

              if (missingCountries != null && missingCountries.length != 0)
                ActiveCountryTree.downloadMapsForIndex(missingCountries, StorageOptions.MAP_OPTION_MAP_AND_CAR_ROUTING);
              if (missingRoutes != null && missingRoutes.length != 0)
                ActiveCountryTree.downloadMapsForIndex(missingRoutes, StorageOptions.MAP_OPTION_CAR_ROUTING);

              if (mContainer != null)
                mContainer.showDownloader(true);
            }

            @Override
            public void onOk()
            {
              if (ResultCodesHelper.isDownloadable(resultCode))
              {
                cancel();

                if (mContainer != null)
                  mContainer.showDownloader(false);
              }
            }
          });

          fragment.show(mContainer.getActivity().getSupportFragmentManager(), fragment.getClass().getSimpleName());
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
          mLastBuildProgress = (int)progress;
          updateProgress();
        }
      });
    }
  };

  private RoutingController()
  {
    Framework.nativeSetRoutingListener(mRoutingListener);
    Framework.nativeSetRouteProgressListener(mRoutingProgressListener);
  }

  public static RoutingController get()
  {
    return sInstance;
  }

  private void setState(State newState)
  {
    Log.d(TAG, "[S] State: " + mState + " -> " + newState + ", BuildState: " + mBuildState);
    mState = newState;

    if (mContainer != null)
      mContainer.updateMenu();
  }

  private void setBuildState(BuildState newState)
  {
    Log.d(TAG, "[B] State: " + mState + ", BuildState: " + mBuildState + " -> " + newState);
    mBuildState = newState;
  }

  private void updateProgress()
  {
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
    Log.d(TAG, "attach");

    if (mContainer != null)
      throw new IllegalStateException("Must be detached before attach()");

    mContainer = container;
  }

  public void restore()
  {
    if (isPlanning())
    {
      // TODO: Restore search fragment
      showRoutePlan();
    }

    mContainer.showNavigation(isNavigating());
    mContainer.updateMenu();
  }

  public void detach()
  {
    Log.d(TAG, "detach");

    mContainer = null;
    mStartButton = null;
  }

  private void build()
  {
    Log.d(TAG, "build");

    mLastBuildProgress = 0;
    setBuildState(BuildState.BUILDING);
    updatePlan();

    Framework.nativeBuildRoute(mStartPoint.getLat(), mStartPoint.getLon(), mEndPoint.getLat(), mEndPoint.getLon());
  }

  private void showDisclaimer(final MapObject endPoint)
  {
    StringBuilder builder = new StringBuilder();
    for (int resId : new int[] { R.string.dialog_routing_disclaimer_priority, R.string.dialog_routing_disclaimer_precision,
                                 R.string.dialog_routing_disclaimer_recommendations, R.string.dialog_routing_disclaimer_beware })
      builder.append(MwmApplication.get().getString(resId)).append("\n\n");

    new AlertDialog.Builder(mContainer.getActivity())
        .setTitle(R.string.dialog_routing_disclaimer_title)
        .setMessage(builder.toString())
        .setCancelable(false)
        .setNegativeButton(R.string.cancel, null)
        .setPositiveButton(R.string.ok, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            Config.acceptRoutingDisclaimer();
            prepare(endPoint);
          }
        }).show();
  }

  public void prepare(@Nullable MapObject endPoint)
  {
    Log.d(TAG, "prepare (" + (endPoint == null ? "route)" : "p2p)"));

    cancel();
    if (!Config.isRoutingDisclaimerAccepted())
    {
      showDisclaimer(endPoint);
      return;
    }

    if (!LocationState.isTurnedOn())
    {
      mRoutingListener.onRoutingEvent(ResultCodesHelper.NO_POSITION, null, null);
      return;
    }

    mStartPoint = LocationHelper.INSTANCE.getMyPosition();
    if (mStartPoint == null)
    {
      mRoutingListener.onRoutingEvent(ResultCodesHelper.NO_POSITION, null, null);
      return;
    }

    mEndPoint = endPoint;
    setState(State.PREPARE);

    if (mEndPoint != null)
      mLastRouterType = Framework.nativeGetBestRouter(mStartPoint.getLat(), mStartPoint.getLon(),
                                                      mEndPoint.getLat(), mEndPoint.getLon());
    Framework.nativeSetRouter(mLastRouterType);

    if (mContainer != null)
      mContainer.showRoutePlan(true, new Runnable()
      {
        @Override
        public void run()
        {
          if (mEndPoint == null)
            updatePlan();
          else
            build();
        }
      });
  }

  public void start()
  {
    Log.d(TAG, "start");

    setState(State.NAVIGATION);
    Framework.nativeFollowRoute();

    if (mContainer != null)
    {
      mContainer.showRoutePlan(false, null);
      mContainer.showNavigation(true);
    }
  }

  private void updatePlan()
  {
    updateProgress();
    updateStartButton();
  }

  private void updateStartButton()
  {
    Log.d(TAG, "updateStartButton" + (mStartButton == null ? ": SKIP" : ""));

    if (mStartButton == null)
      return;

    mStartButton.setEnabled(mState == State.PREPARE &&
                            mBuildState == BuildState.BUILT);
  }

  public void setStartButton(@Nullable View button)
  {
    Log.d(TAG, "setStartButton");
    mStartButton = button;
    updateStartButton();
  }

  private void cancelInternal()
  {
    mStartPoint = null;
    mEndPoint = null;
    mWaitingPoiPickSlot = NO_SLOT;

    setBuildState(BuildState.NONE);
    setState(State.NONE);

    Framework.nativeCloseRouting();
  }

  public boolean cancel()
  {
    if (isPlanning())
    {
      Log.d(TAG, "cancel: planning");

      cancelInternal();
      if (mContainer != null)
        mContainer.showRoutePlan(false, null);
      return true;
    }

    if (isNavigating())
    {
      Log.d(TAG, "cancel: navigating");

      cancelInternal();
      if (mContainer != null)
      {
        mContainer.showNavigation(false);
        mContainer.updateMenu();
      }
      return true;
    }

    Log.d(TAG, "cancel: none");
    return false;
  }

  public boolean cancelPlanning()
  {
    Log.d(TAG, "cancelPlanning");

    if (isPlanning())
    {
      cancel();
      return true;
    }

    return false;
  }

  // Planning UI is visible, no navigation active
  public boolean isPlanning()
  {
    return (mState == State.PREPARE);
  }

  // Navigation is active
  public boolean isNavigating()
  {
    return (mState == State.NAVIGATION);
  }

  // Route building is in progress
  public boolean isBuilding()
  {
    return (mState == State.PREPARE && mBuildState == BuildState.BUILDING);
  }

  public boolean isWaitingPoiPick()
  {
    return (mWaitingPoiPickSlot != NO_SLOT);
  }

  public BuildState getBuildState()
  {
    return mBuildState;
  }

  public MapObject getStartPoint()
  {
    return mStartPoint;
  }

  public MapObject getEndPoint()
  {
    return mEndPoint;
  }

  private void checkAndBuildRoute()
  {
    if (mContainer != null)
    {
      if (isWaitingPoiPick())
        showRoutePlan();
      else
        mContainer.updatePoints();
    }

    if (mStartPoint != null && mEndPoint != null)
      build();
  }

  private boolean setStartFromMyPosition()
  {
    Log.d(TAG, "setStartFromMyPosition");

    MapObject my = LocationHelper.INSTANCE.getMyPosition();
    if (my == null)
    {
      Log.d(TAG, "setStartFromMyPosition: no my position - skip");
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
    Log.d(TAG, "setStartPoint");

    if (MapObject.same(mStartPoint, point))
    {
      Log.d(TAG, "setStartPoint: skip the same starting point");
      return false;
    }

    if (point != null && point.sameAs(mEndPoint))
    {
      if (mStartPoint == null)
      {
        Log.d(TAG, "setStartPoint: skip because starting point is empty");
        return false;
      }

      Log.d(TAG, "setStartPoint: swap with end point");
      mEndPoint = mStartPoint;
    }

    mStartPoint = point;
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
    Log.d(TAG, "setEndPoint");

    if (MapObject.same(mEndPoint, point))
    {
      if (mStartPoint == null)
        return setStartFromMyPosition();

      Log.d(TAG, "setEndPoint: skip the same end point");
      return false;
    }

    if (point != null && point.sameAs(mStartPoint))
    {
      if (mEndPoint == null)
      {
        Log.d(TAG, "setEndPoint: skip because end point is empty");
        return false;
      }

      Log.d(TAG, "setEndPoint: swap with starting point");
      mStartPoint = mEndPoint;
    }

    mEndPoint = point;

    if (mStartPoint == null)
      return setStartFromMyPosition();

    checkAndBuildRoute();
    return true;
  }

  public void swapPoints()
  {
    Log.d(TAG, "swapPoints");

    MapObject point = mStartPoint;
    mStartPoint = mEndPoint;
    mEndPoint = point;

    checkAndBuildRoute();
  }

  public void setRouterType(int router)
  {
    Log.d(TAG, "setRouterType: " + mLastRouterType + " -> " + router);

    if (router == mLastRouterType)
      return;

    mLastRouterType = router;
    Framework.nativeSetRouter(router);

    if (mStartPoint != null && mEndPoint != null)
      build();
  }

  public void searchPoi(int slotId)
  {
    Log.d(TAG, "searchPoi: " + slotId);
    mWaitingPoiPickSlot = slotId;
    mContainer.showSearch();
    mContainer.updateMenu();
  }

  private void onPoiSelectedInternal(@Nullable MapObject point)
  {
    if (mContainer == null)
      return;

    mContainer.updateMenu();

    if (point != null)
    {
      boolean set;
      if (mWaitingPoiPickSlot == 1)
        set = setStartPoint(point);
      else
        set = setEndPoint(point);

      if (set)
        return;
    }

    showRoutePlan();
  }

  public void onPoiSelected(@Nullable MapObject point)
  {
    onPoiSelectedInternal(point);
    mContainer.updatePoints();
    mWaitingPoiPickSlot = NO_SLOT;
  }

  public static CharSequence formatRoutingTime(int seconds)
  {
    long minutes = TimeUnit.SECONDS.toMinutes(seconds) % 60;
    long hours = TimeUnit.SECONDS.toHours(seconds);
    if (hours == 0 && minutes == 0)
      // One minute is added to estimated time to destination point to prevent displaying zero minutes left
      minutes++;

    return hours == 0 ? Utils.formatUnitsText(R.dimen.text_size_routing_number,
                                              R.dimen.text_size_routing_dimension,
                                              String.valueOf(minutes), "min")
                      : TextUtils.concat(Utils.formatUnitsText(R.dimen.text_size_routing_number,
                                                               R.dimen.text_size_routing_dimension,
                                                               String.valueOf(hours), "h "),
                                         Utils.formatUnitsText(R.dimen.text_size_routing_number,
                                                               R.dimen.text_size_routing_dimension,
                                                               String.valueOf(minutes), "min"));
  }

  public static String formatArrivalTime(int seconds)
  {
    Calendar current = Calendar.getInstance();
    current.add(Calendar.SECOND, seconds);
    return String.format(Locale.US, "%d:%02d", current.get(Calendar.HOUR_OF_DAY), current.get(Calendar.MINUTE));
  }
}
