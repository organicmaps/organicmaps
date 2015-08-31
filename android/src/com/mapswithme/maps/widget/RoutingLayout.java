package com.mapswithme.maps.widget;

import android.content.Context;
import android.content.DialogInterface;
import android.location.Location;
import android.os.Build;
import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;
import android.support.v7.app.AlertDialog;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.AbsoluteSizeSpan;
import android.util.AttributeSet;
import android.view.View;
import android.widget.ImageView;
import android.widget.RadioGroup;
import android.widget.RelativeLayout;
import android.widget.TextView;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.LocationState;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.routing.RoutingInfo;
import com.mapswithme.maps.routing.RoutingResultCodesProcessor;
import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.AlohaHelper;

import java.util.Calendar;
import java.util.concurrent.TimeUnit;

/**
 * Layout for routing setup & turn instruction box.
 */
public class RoutingLayout extends RelativeLayout implements View.OnClickListener
{
  private WheelProgressView mWvProgress;
  private TextView mTvPlanning;
  private View mIvCancelRouteBuild;
  private TextView mTvPrepareTime;
  private TextView mTvTotalDistance;
  private TextView mTvTotalTime;
  private ImageView mIvTurn;
  private TextView mTvExitNum;
  private View mNextTurn;
  private ImageView mIvNextTurn;
  private TextView mTvTurnDistance;
  private TextView mTvPrepareDistance;
  private MapObject mEndPoint;
  private View mLayoutSetupRouting;
  private View mLayoutTurnInstructions;
  private View mBtnStart;
  private FlatProgressView mFpRouteProgress;
  private RadioGroup mRgRouterType;
  private TextView mTvNextStreet;
  private TextView mTvArrivalTime;

  private double mNorth;
  private RoutingInfo mCachedRoutingInfo;

  public enum State
  {
    HIDDEN,
    PREPARING,
    ROUTE_BUILT,
    ROUTE_BUILD_ERROR,
    TURN_INSTRUCTIONS
  }

  private State mState = State.HIDDEN;

  public interface ActionListener
  {
    void onCloseRouting();

    void onStartRouteFollow();

    void onRouteTypeChange(int type);
  }

  private ActionListener mListener;

  public RoutingLayout(Context context)
  {
    this(context, null, 0);
  }

  public RoutingLayout(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public RoutingLayout(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs, defStyleAttr);
    inflate(getContext(), R.layout.layout_routing_full, this);
    setClipToPadding(false);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
      setElevation(UiUtils.dimen(R.dimen.appbar_elevation));

    if (isInEditMode())
      return;

    initViews();
    UiUtils.hide(this, mLayoutSetupRouting, mBtnStart, mLayoutTurnInstructions);
  }

  public void setListener(ActionListener listener)
  {
    mListener = listener;
  }

  public void refreshAzimuth(double north)
  {
    mNorth = north;
    if (mState == State.TURN_INSTRUCTIONS)
      refreshTurnInstructions();
  }

  private void initViews()
  {
    mLayoutSetupRouting = findViewById(R.id.layout__routing_setup);
    mWvProgress = (WheelProgressView) mLayoutSetupRouting.findViewById(R.id.wp__routing_progress);
    mWvProgress.setOnClickListener(this);
    mTvPlanning = (TextView) mLayoutSetupRouting.findViewById(R.id.tv__planning_route);
    mRgRouterType = (RadioGroup) mLayoutSetupRouting.findViewById(R.id.rg__router);
    mRgRouterType.findViewById(R.id.rb__vehicle).setOnClickListener(this);
    mRgRouterType.findViewById(R.id.rb__pedestrian).setOnClickListener(this);
    mIvCancelRouteBuild = mLayoutSetupRouting.findViewById(R.id.iv__routing_close);
    mIvCancelRouteBuild.setOnClickListener(this);
    mTvPrepareDistance = (TextView) mLayoutSetupRouting.findViewById(R.id.tv__routing_distance);
    mTvPrepareTime = (TextView) mLayoutSetupRouting.findViewById(R.id.tv__routing_time);
    mBtnStart = mLayoutSetupRouting.findViewById(R.id.btn__start_routing);
    mBtnStart.setOnClickListener(this);

    mLayoutTurnInstructions = findViewById(R.id.layout__turn_instructions);
    mTvTotalDistance = (TextView) mLayoutTurnInstructions.findViewById(R.id.tv__total_distance);
    mTvTotalTime = (TextView) mLayoutTurnInstructions.findViewById(R.id.tv__total_time);
    mTvArrivalTime = (TextView) mLayoutTurnInstructions.findViewById(R.id.tv__arrival_time);
    mIvTurn = (ImageView) mLayoutTurnInstructions.findViewById(R.id.iv__turn);
    mTvExitNum = (TextView) mLayoutTurnInstructions.findViewById(R.id.tv__exit_num);
    mNextTurn = findViewById(R.id.next_turn);
    mIvNextTurn = (ImageView) mNextTurn.findViewById(R.id.iv__next_turn);
    mTvTurnDistance = (TextView) mLayoutTurnInstructions.findViewById(R.id.tv__turn_distance);
    mLayoutTurnInstructions.findViewById(R.id.btn__close).setOnClickListener(this);
    mFpRouteProgress = (FlatProgressView) mLayoutTurnInstructions.findViewById(R.id.fp__route_progress);
    mTvNextStreet = (TextView) mLayoutTurnInstructions.findViewById(R.id.tv__next_street);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.btn__close:
      AlohaHelper.logClick(AlohaHelper.ROUTING_CLOSE);
      setState(State.HIDDEN, true);
      mListener.onCloseRouting();
      break;
    case R.id.iv__routing_close:
      AlohaHelper.logClick(AlohaHelper.ROUTING_GO_CLOSE);
      setState(State.HIDDEN, true);
      mListener.onCloseRouting();
      break;
    case R.id.wp__routing_progress:
      AlohaHelper.logClick(AlohaHelper.ROUTING_PROGRESS_CLOSE);
      setState(State.HIDDEN, true);
      mListener.onCloseRouting();
      break;
    case R.id.btn__start_routing:
      AlohaHelper.logClick(AlohaHelper.ROUTING_GO);
      setState(State.TURN_INSTRUCTIONS, true);
      Framework.nativeFollowRoute();
      mListener.onStartRouteFollow();
      break;
    case R.id.rb__pedestrian:
      AlohaHelper.logClick(AlohaHelper.ROUTING_PEDESTRIAN_SET);
      Framework.setRouter(Framework.ROUTER_TYPE_PEDESTRIAN);
      mListener.onRouteTypeChange(Framework.ROUTER_TYPE_PEDESTRIAN);
      setState(State.PREPARING, true);
      break;
    case R.id.rb__vehicle:
      AlohaHelper.logClick(AlohaHelper.ROUTING_VEHICLE_SET);
      Framework.setRouter(Framework.ROUTER_TYPE_VEHICLE);
      mListener.onRouteTypeChange(Framework.ROUTER_TYPE_VEHICLE);
      setState(State.PREPARING, true);
      break;
    }
  }

  public void setState(State state, boolean animated)
  {
    // TODO show routing progress after its implemented.
    mState = state;
    switch (mState)
    {
    case HIDDEN:
      if (animated)
        UiUtils.disappearSlidingUp(this, null);
      else
        UiUtils.hide(this);
      UiUtils.hide(mBtnStart, mTvExitNum);
      Framework.nativeCloseRouting();
      mEndPoint = null;
      break;
    case PREPARING:
      if (mEndPoint == null)
        throw new IllegalStateException("End point should be not null to prepare routing");

      Framework.nativeCloseRouting();
      UiUtils.show(mLayoutSetupRouting, mWvProgress, mTvPlanning);
      // TODO (marchuk): Uncomment after the second turn notification is fixed.
//      UiUtils.hide(mLayoutTurnInstructions, mTvPrepareDistance, mTvPrepareTime, mIvCancelRouteBuild, mNextTurn);
      UiUtils.hide(mLayoutTurnInstructions, mTvPrepareDistance, mTvPrepareTime, mIvCancelRouteBuild);
      mTvPlanning.setText(R.string.routing_planning);
      mWvProgress.setProgress(0);
      if (animated)
      {
        UiUtils.appearSlidingDown(this, null);
        UiUtils.disappearSlidingUp(mBtnStart, null);
      }
      else
      {
        UiUtils.show(this);
        UiUtils.hide(mBtnStart);
      }
      buildRoute();
      break;
    case ROUTE_BUILT:
      UiUtils.show(this, mLayoutSetupRouting, mTvPrepareDistance, mTvPrepareTime, mIvCancelRouteBuild);
      // TODO (marchuk): Uncomment after the second turn notification is fixed.
//      UiUtils.hide(mLayoutTurnInstructions, mWvProgress, mTvPlanning, mNextTurn);
      UiUtils.hide(mLayoutTurnInstructions, mWvProgress, mTvPlanning);
      if (animated)
        UiUtils.appearSlidingDown(mBtnStart, null);
      else
        UiUtils.show(mBtnStart);

      refreshRouteSetup();
      break;
    case ROUTE_BUILD_ERROR:
      UiUtils.show(mLayoutSetupRouting, mIvCancelRouteBuild, mTvPlanning);
      // TODO (marchuk): Uncomment after the second turn notification is fixed.
//      UiUtils.hide(mLayoutTurnInstructions, mTvPrepareDistance, mTvPrepareTime, mWvProgress, mNextTurn);
      UiUtils.hide(mLayoutTurnInstructions, mTvPrepareDistance, mTvPrepareTime, mWvProgress);

      mTvPlanning.setText(R.string.routing_planning_error);
      break;
    case TURN_INSTRUCTIONS:
      UiUtils.show(this, mLayoutTurnInstructions);
      UiUtils.disappearSlidingUp(mLayoutSetupRouting, null);
      refreshTurnInstructions();
      break;
    }
  }

  public State getState()
  {
    return mState;
  }

  public void setEndPoint(MapObject mapObject)
  {
    mEndPoint = mapObject;
    checkBestRouter();
  }

  public MapObject getEndPoint()
  {
    return mEndPoint;
  }

  public void updateRouteInfo()
  {
    mCachedRoutingInfo = Framework.nativeGetRouteFollowingInfo();
    if (mState == State.TURN_INSTRUCTIONS)
      refreshTurnInstructions();
  }

  public void setRouteBuildingProgress(float progress)
  {
    mWvProgress.setProgress((int) progress);
  }

  private void refreshTurnInstructions()
  {
    if (mCachedRoutingInfo == null)
      return;

    if (Framework.getRouter() == Framework.ROUTER_TYPE_VEHICLE)
      refreshVehicleInfo(mCachedRoutingInfo);
    else
      refreshPedestrianAzimutAndDistance(mCachedRoutingInfo);

    mTvTotalTime.setText(formatRoutingTime(mCachedRoutingInfo.totalTimeInSeconds));
    mTvTotalDistance.setText(buildSpannedText(UiUtils.dimen(R.dimen.text_size_routing_number), UiUtils.dimen(R.dimen.text_size_routing_dimension),
                                              mCachedRoutingInfo.distToTarget, mCachedRoutingInfo.targetUnits));
    mTvArrivalTime.setText(formatArrivalTime(mCachedRoutingInfo.totalTimeInSeconds));
    UiUtils.setTextAndHideIfEmpty(mTvNextStreet, mCachedRoutingInfo.nextStreet);
    mFpRouteProgress.setProgress((int) mCachedRoutingInfo.completionPercent);
  }

  private void refreshVehicleInfo(RoutingInfo routingInfo)
  {
    mTvTurnDistance.setText(buildSpannedText(UiUtils.dimen(R.dimen.text_size_display_1), UiUtils.dimen(R.dimen.text_size_toolbar),
                                             routingInfo.distToTurn, routingInfo.turnUnits));
    routingInfo.vehicleTurnDirection.setTurnDrawable(mIvTurn);
    if (RoutingInfo.VehicleTurnDirection.isRoundAbout(routingInfo.vehicleTurnDirection))
      UiUtils.setTextAndShow(mTvExitNum, String.valueOf(routingInfo.exitNum));
    else
      UiUtils.hide(mTvExitNum);

    // TODO (marchuk): Uncomment after the second turn notification is fixed.
//    if (routingInfo.vehicleNextTurnDirection.containsNextTurn())
//    {
//      UiUtils.appearSlidingDown(mNextTurn, null);
//      routingInfo.vehicleNextTurnDirection.setNextTurnDrawable(mIvNextTurn);
//    }
//    else
//      UiUtils.disappearSlidingUp(mNextTurn, null);
  }

  private void refreshPedestrianAzimutAndDistance(RoutingInfo info)
  {
    Location location = LocationHelper.INSTANCE.getLastLocation();
    DistanceAndAzimut distanceAndAzimut = Framework.nativeGetDistanceAndAzimutFromLatLon(
        info.pedestrianNextDirection.getLatitude(), info.pedestrianNextDirection.getLongitude(), location.getLatitude(), location.getLongitude(), mNorth);

    String[] splitDistance = distanceAndAzimut.getDistance().split(" ");
    mTvTurnDistance.setText(buildSpannedText(UiUtils.dimen(R.dimen.text_size_display_1), UiUtils.dimen(R.dimen.text_size_toolbar),
                                             splitDistance[0], splitDistance[1]));

    if (info.pedestrianTurnDirection != null)
      info.pedestrianTurnDirection.setTurnDrawable(mIvTurn, distanceAndAzimut);
  }

  private void refreshRouteSetup()
  {
    mCachedRoutingInfo = Framework.nativeGetRouteFollowingInfo();
    if (mCachedRoutingInfo == null)
      return;

    mTvPrepareDistance.setText(buildSpannedText(UiUtils.dimen(R.dimen.text_size_routing_number), UiUtils.dimen(R.dimen.text_size_routing_dimension),
                                                mCachedRoutingInfo.distToTarget, mCachedRoutingInfo.targetUnits));
    mTvPrepareTime.setText(formatRoutingTime(mCachedRoutingInfo.totalTimeInSeconds));
  }

  private static SpannableStringBuilder buildSpannedText(int mainTextSize, int unitsTextSize, String main, String units)
  {
    SpannableStringBuilder builder = new SpannableStringBuilder(main).append(" ").append(units);
    builder.setSpan(new AbsoluteSizeSpan(mainTextSize, false), 0, main.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.setSpan(new AbsoluteSizeSpan(unitsTextSize, false), main.length(), builder.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);

    return builder;
  }

  private static CharSequence formatRoutingTime(int seconds)
  {
    long minutes = TimeUnit.SECONDS.toMinutes(seconds) % 60;
    long hours = TimeUnit.SECONDS.toHours(seconds);
    if (hours == 0 && minutes == 0)
      // one minute is added to estimated time to destination point to prevent displaying zero minutes left
      minutes++;

    return hours == 0 ?
           buildSpannedText(UiUtils.dimen(R.dimen.text_size_routing_number), UiUtils.dimen(R.dimen.text_size_routing_dimension), String.valueOf(minutes), "min") :
           TextUtils.concat(buildSpannedText(UiUtils.dimen(R.dimen.text_size_routing_number), UiUtils.dimen(R.dimen.text_size_routing_dimension),
                                             String.valueOf(hours), "h "),
                            buildSpannedText(UiUtils.dimen(R.dimen.text_size_routing_number), UiUtils.dimen(R.dimen.text_size_routing_dimension),
                                             String.valueOf(minutes), "min"));
  }

  private static String formatArrivalTime(int seconds)
  {
    Calendar current = Calendar.getInstance();
    current.add(Calendar.SECOND, seconds);
    return String.format("%d:%02d", current.get(Calendar.HOUR_OF_DAY), current.get(Calendar.MINUTE));
  }

  private void buildRoute()
  {
    if (!Config.isRoutingDisclaimerAccepted())
    {
      showRoutingDisclaimer();
      return;
    }

    if (!LocationState.isTurnedOn())
    {
      onMissingLocation();
      return;
    }

    Location location = LocationHelper.INSTANCE.getLastLocation();
    if (location == null)
    {
      // TODO remove that hack after proper route reconstruction logic will be finished
      setState(State.HIDDEN, false);
      return;
    }
    Framework.nativeBuildRoute(location.getLatitude(), location.getLongitude(), mEndPoint.getLat(), mEndPoint.getLon());
  }

  private void checkBestRouter()
  {
    final Location location = LocationHelper.INSTANCE.getLastLocation();
    if (location == null || mEndPoint == null)
      return;

    final int bestRouter = Framework.nativeGetBestRouter(location.getLatitude(), location.getLongitude(), mEndPoint.getLat(), mEndPoint.getLon());
    mRgRouterType.check(bestRouter == Framework.ROUTER_TYPE_PEDESTRIAN ? R.id.rb__pedestrian : R.id.rb__vehicle);
    Framework.setRouter(bestRouter);
  }

  private void showRoutingDisclaimer()
  {
    StringBuilder builder = new StringBuilder();
    for (int resId : new int[]{R.string.dialog_routing_disclaimer_priority, R.string.dialog_routing_disclaimer_precision,
        R.string.dialog_routing_disclaimer_recommendations, R.string.dialog_routing_disclaimer_beware})
      builder.append(getContext().getString(resId)).append("\n\n");

    new AlertDialog.Builder(getContext())
        .setTitle(R.string.dialog_routing_disclaimer_title)
        .setMessage(builder.toString())
        .setCancelable(false)
        .setPositiveButton(getContext().getString(R.string.ok), new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dlg, int which)
          {
            Config.acceptRoutingDisclaimer();
            dlg.dismiss();
            buildRoute();
          }
        })
        .setNegativeButton(getContext().getString(R.string.cancel), null)
        .show();
  }

  private void onMissingLocation()
  {
    Context context = getContext();
    if (context instanceof Framework.RoutingListener)
      ((Framework.RoutingListener) context).onRoutingEvent(RoutingResultCodesProcessor.NO_POSITION, null, null);
  }

  @Override
  protected Parcelable onSaveInstanceState()
  {
    Parcelable parentState = super.onSaveInstanceState();
    SavedState savedState = new SavedState(parentState);
    savedState.object = getEndPoint();
    savedState.routingStateOrdinal = mState.ordinal();
    return savedState;
  }

  @Override
  protected void onRestoreInstanceState(Parcelable state)
  {
    SavedState savedState = (SavedState) state;
    super.onRestoreInstanceState(savedState.getSuperState());
    setEndPoint(savedState.object);

    // if route was build but it was lost before state was restored - we should rebuild again from scratch
    int correctOrdinal;
    if (savedState.routingStateOrdinal > State.PREPARING.ordinal() && !Framework.nativeIsRouteBuilt())
      correctOrdinal = State.PREPARING.ordinal();
    else
      correctOrdinal = savedState.routingStateOrdinal;

    setState(State.values()[correctOrdinal], false);
  }

  static class SavedState extends BaseSavedState
  {
    int routingStateOrdinal;
    MapObject object;

    public SavedState(Parcel source)
    {
      super(source);
      routingStateOrdinal = source.readInt();
      object = source.readParcelable(MapObject.class.getClassLoader());
    }

    public SavedState(Parcelable superState)
    {
      super(superState);
    }

    @Override
    public void writeToParcel(@NonNull Parcel dest, int flags)
    {
      super.writeToParcel(dest, flags);
      dest.writeInt(routingStateOrdinal);
      dest.writeParcelable(object, 0);
    }

    public static final Creator<SavedState> CREATOR = new Creator<SavedState>()
    {
      @Override
      public SavedState createFromParcel(Parcel source)
      {
        return new SavedState(source);
      }

      @Override
      public SavedState[] newArray(int size)
      {
        return new SavedState[0];
      }
    };
  }
}
