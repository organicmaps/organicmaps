package app.organicmaps.sdk.routing;

import androidx.annotation.DrawableRes;
import app.organicmaps.sdk.R;

/**
 * IMPORTANT : Order of enum values MUST BE the same as native CarDirection enum.
 */
public enum CarDirection
{
  NO_TURN(R.drawable.ic_turn_straight, 0),
  GO_STRAIGHT(R.drawable.ic_turn_straight, 0),

  TURN_RIGHT(R.drawable.ic_turn_right, R.drawable.ic_then_right),
  TURN_SHARP_RIGHT(R.drawable.ic_turn_right_sharp, R.drawable.ic_then_right_sharp),
  TURN_SLIGHT_RIGHT(R.drawable.ic_turn_right_slight, R.drawable.ic_then_right_slight),

  TURN_LEFT(R.drawable.ic_turn_left, R.drawable.ic_then_left),
  TURN_SHARP_LEFT(R.drawable.ic_turn_left_sharp, R.drawable.ic_then_left_sharp),
  TURN_SLIGHT_LEFT(R.drawable.ic_turn_left_slight, R.drawable.ic_then_left_slight),

  U_TURN_LEFT(R.drawable.ic_turn_uleft, R.drawable.ic_then_uleft),
  U_TURN_RIGHT(R.drawable.ic_turn_uright, R.drawable.ic_then_uright),

  ENTER_ROUND_ABOUT(R.drawable.ic_turn_round, R.drawable.ic_turn_round),
  LEAVE_ROUND_ABOUT(R.drawable.ic_turn_round, R.drawable.ic_turn_round),
  STAY_ON_ROUND_ABOUT(R.drawable.ic_turn_round, R.drawable.ic_turn_round),

  START_AT_THE_END_OF_STREET(0, 0),
  REACHED_YOUR_DESTINATION(R.drawable.ic_turn_finish, R.drawable.ic_then_finish),

  EXIT_HIGHWAY_TO_LEFT(R.drawable.ic_exit_highway_to_left, R.drawable.ic_then_exit_highway_to_left),
  EXIT_HIGHWAY_TO_RIGHT(R.drawable.ic_exit_highway_to_right, R.drawable.ic_then_exit_highway_to_right);

  private final int mTurnRes;
  private final int mNextTurnRes;

  CarDirection(@DrawableRes int mainResId, @DrawableRes int nextResId)
  {
    mTurnRes = mainResId;
    mNextTurnRes = nextResId;
  }

  @DrawableRes
  public int getTurnRes(final int exitNum)
  {
    if (isRoundAbout(this))
      return getRoundaboutRes(exitNum);
    return mTurnRes;
  }

  @DrawableRes
  public int getNextTurnRes()
  {
    return mNextTurnRes;
  }

  public boolean containsNextTurn()
  {
    return mNextTurnRes != 0;
  }

  public static boolean isRoundAbout(CarDirection turn)
  {
    return turn == ENTER_ROUND_ABOUT || turn == LEAVE_ROUND_ABOUT || turn == STAY_ON_ROUND_ABOUT;
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
