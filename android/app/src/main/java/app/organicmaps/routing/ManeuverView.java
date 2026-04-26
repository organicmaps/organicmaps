package app.organicmaps.routing;

import android.content.Context;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.RoutingInfo;
import app.organicmaps.sdk.widget.roadshield.RoadShieldUtils;
import app.organicmaps.sdk.widgets.lanes.LanesView;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;

/**
 * Self-contained navigation maneuver card.
 *
 * Layout (top to bottom):
 *   1. Lane guidance row — visible when lane data is available; turn arrow is hidden in this case
 *   2. Turn arrow + distance row
 *   3. Next street name
 *
 * Call {@link #updateVehicle} or {@link #updatePedestrian} each navigation tick.
 */
public class ManeuverView extends LinearLayout
{
  private final ImageView mTurnImage;
  private final TextView mDistance;
  private final View mStreetFrame;
  private final TextView mStreet;
  private final LanesView mLanes;

  public ManeuverView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    setOrientation(VERTICAL);
    LayoutInflater.from(context).inflate(R.layout.view_maneuver, this, true);
    mTurnImage = ViewCompat.requireViewById(this, R.id.maneuver_turn);
    mDistance = ViewCompat.requireViewById(this, R.id.maneuver_distance);
    mStreetFrame = ViewCompat.requireViewById(this, R.id.maneuver_street_frame);
    mStreet = ViewCompat.requireViewById(mStreetFrame, R.id.maneuver_street);
    mLanes = ViewCompat.requireViewById(this, R.id.maneuver_lanes);
  }

  /** Update for vehicle / bicycle routing. Shows lane guidance when available. */
  public void updateVehicle(@NonNull RoutingInfo info)
  {
    mLanes.setLanes(info.lanes);
    final boolean lanesVisible = info.lanes != null && info.lanes.length > 0;
    UiUtils.showIf(!lanesVisible, mTurnImage);
    if (!lanesVisible)
      mTurnImage.setImageResource(info.carDirection.getTurnRes(info.exitNum));
    setDistanceMarginStart(!lanesVisible);
    updateCommon(info);
  }

  /** Update for pedestrian routing. Always shows turn arrow; no lane data. */
  public void updatePedestrian(@NonNull RoutingInfo info)
  {
    mLanes.setLanes(null);
    UiUtils.showIf(true, mTurnImage);
    mTurnImage.setImageResource(info.pedestrianDirection.getTurnRes());
    setDistanceMarginStart(true);
    updateCommon(info);
  }

  private void setDistanceMarginStart(boolean hasTurnImage)
  {
    final LinearLayout.LayoutParams lp = (LinearLayout.LayoutParams) mDistance.getLayoutParams();
    lp.setMarginStart(hasTurnImage ? getResources().getDimensionPixelSize(R.dimen.margin_base) : 0);
    mDistance.setLayoutParams(lp);
  }

  private void updateCommon(@NonNull RoutingInfo info)
  {
    mDistance.setText(Utils.formatDistance(getContext(), info.distToTurn));

    final boolean hasStreet = !TextUtils.isEmpty(info.nextStreet);
    // Sic: don't use UiUtils.showIf() here because View.GONE breaks layout
    // https://github.com/organicmaps/organicmaps/issues/3732
    UiUtils.visibleIf(hasStreet, mStreetFrame);
    if (hasStreet)
      mStreet.setText(RoadShieldUtils.createStreetTextWithShields(
          info.nextStreet, info.nextStreetRoadShields, mStreet.getTextSize()));
  }
}
