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
 *   4. "Then" chip — the maneuver right after the upcoming one, shown when
 *      {@link RoutingInfo#hasNextNextTurn()}
 *
 * The card owns its rounded background and content padding. Window insets (status bar and
 * side display cutouts) are applied by NavigationController on the shared nav_top_frame, which
 * shifts the card and the speed limit together.
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
  private final View mNextNextTurnFrame;
  private final ImageView mNextNextTurnImage;

  public ManeuverView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    setOrientation(VERTICAL);
    // Background on the view itself also makes the elevation shadow follow the rounded
    // outline instead of the rectangular view bounds.
    setBackgroundResource(R.drawable.bg_nav_maneuver_card);
    final int padding = getResources().getDimensionPixelSize(R.dimen.margin_half_plus);
    setPadding(padding, padding, padding, padding);
    LayoutInflater.from(context).inflate(R.layout.view_maneuver, this, true);
    mTurnImage = ViewCompat.requireViewById(this, R.id.maneuver_turn);
    mDistance = ViewCompat.requireViewById(this, R.id.maneuver_distance);
    mStreetFrame = ViewCompat.requireViewById(this, R.id.maneuver_street_frame);
    mStreet = ViewCompat.requireViewById(mStreetFrame, R.id.maneuver_street);
    mLanes = ViewCompat.requireViewById(this, R.id.maneuver_lanes);
    mNextNextTurnFrame = ViewCompat.requireViewById(this, R.id.maneuver_next_next_turn_frame);
    mNextNextTurnImage = ViewCompat.requireViewById(mNextNextTurnFrame, R.id.maneuver_next_next_turn);
  }

  /** Update for vehicle / bicycle routing. Shows lane guidance when available. */
  public void updateVehicle(@NonNull RoutingInfo info)
  {
    mLanes.setLanes(info.lanes, info.lanesTrimmedLeft, info.lanesTrimmedRight);
    final boolean lanesVisible = info.lanes != null && info.lanes.length > 0;
    UiUtils.showIf(!lanesVisible, mTurnImage);
    if (!lanesVisible)
      mTurnImage.setImageResource(info.carDirection.getTurnRes(info.exitNum));

    final boolean showNextNextTurn = info.hasNextNextTurn();
    UiUtils.showIf(showNextNextTurn, mNextNextTurnFrame);
    if (showNextNextTurn)
      mNextNextTurnImage.setImageResource(info.nextCarDirection.getTurnRes());

    updateCommon(info);
  }

  /** Update for pedestrian routing. Always shows turn arrow; no lane data. */
  public void updatePedestrian(@NonNull RoutingInfo info)
  {
    mLanes.setLanes(null);
    UiUtils.show(mTurnImage);
    mTurnImage.setImageResource(info.pedestrianDirection.getTurnRes());
    UiUtils.hide(mNextNextTurnFrame);
    updateCommon(info);
  }

  private void updateCommon(@NonNull RoutingInfo info)
  {
    mDistance.setText(Utils.formatDistance(getContext(), info.distToTurn));

    final boolean hasStreet = !TextUtils.isEmpty(info.nextStreet);
    // INVISIBLE (not GONE) keeps the street row's space, so the card height stays stable
    // while the street name briefly disappears between maneuvers.
    UiUtils.visibleIf(hasStreet, mStreetFrame);
    if (hasStreet)
      mStreet.setText(RoadShieldUtils.createStreetTextWithShields(info.nextStreet, info.nextStreetRoadShields,
                                                                  mStreet.getTextSize()));
  }
}
