package app.organicmaps.sdk.routing;

import androidx.annotation.DrawableRes;
import app.organicmaps.sdk.R;

/**
 * IMPORTANT : Order of enum values MUST BE the same as native PedestrianDirection enum.
 */
public enum PedestrianDirection
{
  NoTurn(R.drawable.ic_turn_straight),
  GoStraight(R.drawable.ic_turn_straight),

  TurnRight(R.drawable.ic_turn_right),
  TurnLeft(R.drawable.ic_turn_left),

  ReachedYourDestination(R.drawable.ic_turn_finish);

  private final int mTurnRes;

  PedestrianDirection(@DrawableRes int mainResId)
  {
    mTurnRes = mainResId;
  }

  @DrawableRes
  public int getTurnRes()
  {
    return mTurnRes;
  }
}
