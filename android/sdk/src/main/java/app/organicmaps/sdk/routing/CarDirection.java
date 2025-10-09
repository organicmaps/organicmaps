package app.organicmaps.sdk.routing;

import androidx.annotation.DrawableRes;
import app.organicmaps.sdk.R;

/**
 * IMPORTANT : Order of enum values MUST BE the same as native CarDirection enum.
 */
public enum CarDirection
{
  NoTurn(R.drawable.ic_turn_straight),
  GoStraight(R.drawable.ic_turn_straight),

  TurnRight(R.drawable.ic_turn_right),
  TurnSharpRight(R.drawable.ic_turn_right_sharp),
  TurnSlightRight(R.drawable.ic_turn_right_slight),

  TurnLeft(R.drawable.ic_turn_left),
  TurnSharpLeft(R.drawable.ic_turn_left_sharp),
  TurnSlightLeft(R.drawable.ic_turn_left_slight),

  UTurnLeft(R.drawable.ic_turn_uleft),
  UTurnRight(R.drawable.ic_turn_uright),

  EnterRoundAbout(R.drawable.ic_turn_round),
  LeaveRoundAbout(R.drawable.ic_turn_round),
  StayOnRoundAbout(R.drawable.ic_turn_round),

  StartAtEndOfStreet(R.drawable.ic_turn_straight),
  ReachedYourDestination(R.drawable.ic_turn_finish),

  ExitHighwayToLeft(R.drawable.ic_exit_highway_to_left),
  ExitHighwayToRight(R.drawable.ic_exit_highway_to_right);

  @DrawableRes
  private final int mTurnRes;

  CarDirection(@DrawableRes int mainResId)
  {
    mTurnRes = mainResId;
  }

  @DrawableRes
  public int getTurnRes()
  {
    return mTurnRes;
  }

  @DrawableRes
  public int getTurnRes(final int exitNum)
  {
    if (isRoundAbout(this))
      return getRoundaboutRes(exitNum);
    return getTurnRes();
  }

  public static boolean isRoundAbout(CarDirection turn)
  {
    return turn == EnterRoundAbout || turn == LeaveRoundAbout || turn == StayOnRoundAbout;
  }

  @DrawableRes
  private static int getRoundaboutRes(int exitNum)
  {
    return switch (exitNum)
    {
      case 1 -> R.drawable.ic_roundabout_exit_1;
      case 2 -> R.drawable.ic_roundabout_exit_2;
      case 3 -> R.drawable.ic_roundabout_exit_3;
      case 4 -> R.drawable.ic_roundabout_exit_4;
      case 5 -> R.drawable.ic_roundabout_exit_5;
      case 6 -> R.drawable.ic_roundabout_exit_6;
      case 7 -> R.drawable.ic_roundabout_exit_7;
      case 8 -> R.drawable.ic_roundabout_exit_8;
      case 9 -> R.drawable.ic_roundabout_exit_9;
      case 10 -> R.drawable.ic_roundabout_exit_10;
      case 11 -> R.drawable.ic_roundabout_exit_11;
      case 12 -> R.drawable.ic_roundabout_exit_12;
      default -> R.drawable.ic_turn_round;
    };
  }
}
