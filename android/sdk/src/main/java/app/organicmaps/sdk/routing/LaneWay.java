package app.organicmaps.sdk.routing;

import androidx.annotation.DrawableRes;
import app.organicmaps.sdk.R;

public enum LaneWay
{
  None(R.drawable.ic_turn_straight),
  ReverseLeft(R.drawable.ic_turn_uleft),
  SharpLeft(R.drawable.ic_turn_left_sharp),
  Left(R.drawable.ic_turn_left),
  MergeToLeft(R.drawable.ic_turn_left_slight),
  SlightLeft(R.drawable.ic_turn_left_slight),
  Through(R.drawable.ic_turn_straight),
  SlightRight(R.drawable.ic_turn_right_slight),
  MergeToRight(R.drawable.ic_turn_right_slight),
  Right(R.drawable.ic_turn_right),
  SharpRight(R.drawable.ic_turn_right_sharp),
  ReverseRight(R.drawable.ic_turn_uright);

  public final int mTurnRes;

  LaneWay(@DrawableRes int turnRes)
  {
    mTurnRes = turnRes;
  }
}
