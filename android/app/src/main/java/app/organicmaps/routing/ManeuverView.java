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
import app.organicmaps.sdk.routing.roadshield.RoadShieldInfo;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.widget.roadshield.RoadShieldUtils;
import app.organicmaps.sdk.widgets.lanes.LanesView;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;

/**
 * Self-contained navigation maneuver card.
 *
 * Top to bottom: a header row (distance on the left, turn arrow or lane strip on the right), the
 * street name, and a full-width bottom band with the maneuver after this one (see
 * {@link RoutingInfo#hasNextNextTurn()}). Children are clipped to the rounded background so the
 * band follows the card's bottom corners.
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
  private final TextView mNextNextTurnStreet;
  private Boolean mFillToTop;

  public ManeuverView(@NonNull Context context, @Nullable AttributeSet attrs)
  {
    super(context, attrs);
    setOrientation(VERTICAL);
    // clipToOutline makes the shadow and the full-width bottom band follow the rounded corners.
    setBackgroundResource(R.drawable.bg_nav_maneuver_card);
    setClipToOutline(true);
    LayoutInflater.from(context).inflate(R.layout.view_maneuver, this, true);
    mTurnImage = ViewCompat.requireViewById(this, R.id.maneuver_turn);
    mDistance = ViewCompat.requireViewById(this, R.id.maneuver_distance);
    mStreetFrame = ViewCompat.requireViewById(this, R.id.maneuver_street_frame);
    mStreet = ViewCompat.requireViewById(mStreetFrame, R.id.maneuver_street);
    mLanes = ViewCompat.requireViewById(this, R.id.maneuver_lanes);
    mNextNextTurnFrame = ViewCompat.requireViewById(this, R.id.maneuver_next_next_turn_frame);
    mNextNextTurnImage = ViewCompat.requireViewById(mNextNextTurnFrame, R.id.maneuver_next_next_turn);
    mNextNextTurnStreet = ViewCompat.requireViewById(mNextNextTurnFrame, R.id.maneuver_next_next_turn_street);
  }

  /**
   * Edge-to-edge portrait mode: the card fills to the screen top (square top, behind the status
   * bar), content padded down by {@code topInset}. Disabled restores the floating rounded card.
   */
  public void setFillToTop(boolean enabled, int topInset)
  {
    if (mFillToTop == null || mFillToTop != enabled)
    {
      mFillToTop = enabled;
      setBackgroundResource(enabled ? R.drawable.bg_nav_maneuver_card_top_square : R.drawable.bg_nav_maneuver_card);
      // Square-top corners aren't uniform, so clipToOutline can't round the bottom — the drawables do.
      setClipToOutline(!enabled);
    }
    setPaddingRelative(0, enabled ? topInset : 0, 0, 0);
  }

  /** Update for vehicle / bicycle routing. Shows lane guidance when available. */
  public void updateVehicle(@NonNull RoutingInfo info)
  {
    mLanes.setLanes(info.lanes, info.lanesTrimmedLeft, info.lanesTrimmedRight);
    final boolean lanesVisible = info.lanes != null && info.lanes.length > 0;
    UiUtils.showIf(!lanesVisible, mTurnImage);
    if (!lanesVisible)
      mTurnImage.setImageResource(info.carDirection.getTurnRes(info.exitNum));

    // Skip the band when the second maneuver keeps the same street name — repeating it is noise.
    final boolean showNextNextTurn = info.hasNextNextTurn() && !TextUtils.equals(info.nextNextStreet, info.nextStreet);
    UiUtils.showIf(showNextNextTurn, mNextNextTurnFrame);
    if (showNextNextTurn)
    {
      mNextNextTurnImage.setImageResource(info.nextCarDirection.getTurnRes());
      mNextNextTurnStreet.setText(RoadShieldUtils.createStreetTextWithShields(
          info.nextNextStreet, info.nextNextStreetRoadShields, mNextNextTurnStreet.getTextSize()));
    }

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
    // No imminent turn (long straight / just recalculated): fall back to distance-to-destination.
    final boolean hasTurn = info.distToTurn.isValid();
    final Distance distance = hasTurn ? info.distToTurn : info.distToTarget;
    mDistance.setText(Utils.formatDistance(getContext(), distance));

    // Prefer the turn's target street; fall back to the current road so the line isn't blank.
    String street = hasTurn ? info.nextStreet : info.currentStreet;
    RoadShieldInfo shields = hasTurn ? info.nextStreetRoadShields : null;
    if (TextUtils.isEmpty(street))
    {
      street = hasTurn ? info.currentStreet : info.nextStreet;
      shields = hasTurn ? null : info.nextStreetRoadShields;
    }

    final boolean hasStreet = !TextUtils.isEmpty(street);
    // INVISIBLE, not GONE: keeps the row height so the card doesn't jump between maneuvers.
    UiUtils.visibleIf(hasStreet, mStreetFrame);
    if (hasStreet)
      mStreet.setText(RoadShieldUtils.createStreetTextWithShields(street, shields, mStreet.getTextSize()));
  }
}
