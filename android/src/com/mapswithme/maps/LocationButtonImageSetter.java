package com.mapswithme.maps;

import android.graphics.drawable.AnimationDrawable;
import android.graphics.drawable.Drawable;
import android.widget.ImageButton;

import java.util.HashMap;
import java.util.Map;

public class LocationButtonImageSetter
{
  private final static Map<Integer, Integer> STATE_TO_RES = new HashMap<>();

  static
  {
    STATE_TO_RES.put(LocationState.UNKNOWN_POSITION, R.drawable.btn_white_noposition);
    STATE_TO_RES.put(LocationState.NOT_FOLLOW, R.drawable.btn_white_target_off_1);
    STATE_TO_RES.put(LocationState.FOLLOW, R.drawable.btn_white_follow);
    STATE_TO_RES.put(LocationState.ROTATE_AND_FOLLOW, R.drawable.btn_white_direction);
    STATE_TO_RES.put(LocationState.PENDING_POSITION, R.drawable.btn_white_loading_6);
  }

  @SuppressWarnings("deprecation")
  public static void setButtonViewFromState(int state, ImageButton button)
  {
    final int id = STATE_TO_RES.get(state);
    final Drawable draw = button.getResources().getDrawable(id);

    button.setImageDrawable(draw);

    if (draw instanceof AnimationDrawable)
      ((AnimationDrawable) draw).start();
  }
}
