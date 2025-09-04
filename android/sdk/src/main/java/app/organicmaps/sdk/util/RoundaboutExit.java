package app.organicmaps.sdk.util;

import androidx.annotation.DrawableRes;
import app.organicmaps.sdk.R;

public final class RoundaboutExit
{
  @DrawableRes
  public static int getRes(int exitNum)
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
      default -> R.drawable.ic_turn_round;
    };
  }
}
